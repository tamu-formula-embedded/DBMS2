#include "model.h"



float Temp_C_to_K(float T_C)
{
    return T_C + 273.15;
}

float Temp_K_to_C(float T_K)
{
    return T_K - 273.15;
}

//
//  State of charge calc given the starting
//  charge and the accumulated lost charge
//
float F_Q(float Q0, float Qd) // calculates remaining charge
{
    return Q0 - Qd;
}

//  
//  Find the max current of the battery for 
//  OC or RC path based on Q_h and Q_l
//
float F_Q_max(float T_bar, float Q_h, float Q_l)
{
    return ((T_bar - T_L_PT) / (T_H_PT - T_L_PT)) * (Q_h - Q_l) + Q_l;
}

//
//  Calc % SOC based on Q and Q_max for
//  OC and ECM paths
//
float F_Z(float Q, float Q_max)
{
    return CLAMP(Q / Q_max, 0.0f, 1.0f);
}

// 
//  Forward evil LUT (Z -> ?)
//  Where the input is at descrete
//  intervals
//
float LookupFromZ(float z, float T_bar, float* x_t_lows, float* x_t_highs, float n)
{
    T_bar = CLAMP(T_bar, T_L_PT, T_H_PT);

    float i_true = n - z * n;
    int i_low = floorf(i_true);
    int i_high = ceilf(i_true);
    float i_k = i_true - i_low; 

    i_low = CLAMP(i_low, 0, n - 1);
    i_high = CLAMP(i_high, 0, n - 1);

    float a = x_t_highs[i_low];
    float b = x_t_highs[i_high];
    float x_t_low = (b-a) * i_k + a;      // why the mismatch?

    a = x_t_lows[i_high];
    b = x_t_lows[i_low];
    float x_t_high = (b-a) * i_k + a;

    return (x_t_high - x_t_low) * 
            ( (T_bar - T_L_PT) / (T_H_PT - T_L_PT) ) + x_t_low;
}

//
//  Calc OCV 
//
float F_OCV(float z, float T_bar)
{
    return LookupFromZ(z, T_bar, (float*)ocv_t_low, (float*)ocv_t_high, N_OCV_ENTRIES-1);
}

//
//  Calc ECM RC metrics
//
float F_RC_R(float z, float T_bar, int r_n)
{
    r_n = CLAMP(r_n, 0, 3-1);
    return LookupFromZ(z, T_bar, (float*)rc_t_low[r_n], (float*)rc_t_high[r_n], N_RC_ENTRIES-1);
}

//
//  Calc DCIR Resistance
//
float F_DCIR(float V_oc, float I)
{
    // TOOD: clamp I (>0)
    return V_oc / I;
}

// 
//  Calc I Limit
//
float F_I_Limit(float V_min, float R_pack)
{
    // TOOD: clamp R_pack (>0)
    return V_min / R_pack;
}

void ComputeModel(Model* m, float T_bar, float I, float Q0, float Qd, float V_min)
{
    m->Q = F_Q(Q0, Qd);
    
    // OC Path
    float Q_max_oc = F_Q_max(T_bar, Q_BOUND_H_OC, Q_BOUND_L_OC);
    m->z_oc = F_Z(m->Q, Q_max_oc);
    m->V_oc = F_OCV(m->z_oc, T_bar);
    if (I > 0) 
    {
        m->R_oc = F_DCIR(m->V_oc, I);
    }
    else m->R_oc = 0;
    
    // RC/ECM Path
    float Q_max_rc = F_Q_max(T_bar, Q_BOUND_H_RC, Q_BOUND_L_RC);
    m->z_rc = F_Z(m->Q, Q_max_rc);
    m->R0 = F_RC_R(m->z_rc, T_bar, 0);
    m->R1 = F_RC_R(m->z_rc, T_bar, 1);
    m->R2 = F_RC_R(m->z_rc, T_bar, 2);
    m->R_rc = m->R0 + m->R1 + m->R2;

    m->R_pack = MAX(m->R_oc, m->R_rc);

    m->I_lim = F_I_Limit(V_min, m->R_pack);
}

float CalcAvgTemp(DbmsCtx* ctx)
{
    float sum = 0;
    for (int i = 0; i < N_SIDES; i++)
    {
        for (int j = 0; j < N_TEMPS_PER_SIDE; j++)
        {
            sum += ctx->cell_states[i].temps[j];
        }
    }
    return sum / (N_SIDES * N_TEMPS_PER_SIDE);
}

float CalcMinVoltage(DbmsCtx* ctx)
{
    float v_min = 0;
    for (int i = 0; i < N_SIDES; i++)
    {
        for (int j = 0; j < N_GROUPS_PER_SIDE; j++)
        {
            v_min = MIN(v_min, ctx->cell_states[i].voltages[j]);
        }
    }
    return v_min;
}


float CalcMaxTemp(DbmsCtx* ctx)
{
    float t_max = 0;
    for (int i = 0; i < N_SIDES; i++)
    {
        for (int j = 0; j < N_GROUPS_PER_SIDE; j++)
        {
            t_max = MAX(t_max, ctx->cell_states[i].temps[j]);
        }
    }
    return t_max;
}

void UpdateModel(DbmsCtx* ctx)
{
    float min_v = CalcMinVoltage(ctx);
    float max_t = CalcMaxTemp(ctx);
    float avg_t = CalcAvgTemp(ctx);

    float current = ctx->isense.current_ma / 1000.0;
    ctx->qstats.accumulated_loss = ctx->isense.charge_as * 3600.0;    // convert to Ah
    // ^ this should probably be asserted as positive
    CanLog(ctx, "mv: %d\n", (int) (max_t * 1000));
    //temp 
    // ctx->qstats.accumulated_loss = 0;
    // ctx->qstats.initial = 3.9;

    // CanLog(ctx, "Avg T: %d\n",(int)(avg_t * 1000.0));

    ComputeModel(&ctx->model, avg_t, current, ctx->qstats.initial, ctx->qstats.accumulated_loss, min_v);
}

typedef struct {
    uint64_t q0;
    uint32_t set_at;
} InitialChargeBuffer;

int LoadInitialCharge(DbmsCtx* ctx)
{
    int status;
    InitialChargeBuffer buffer;    
    if ((status = LoadStoredObject(ctx, EEPROM_INITIAL_CHARGE, &buffer, sizeof(buffer))) != 0)
    {
        return status;
    }
    ctx->qstats.initial = (float)(buffer.q0 / 1e6);
    ctx->qstats.initial_set_ts = buffer.set_at;
    return 0;
}

int SaveInitialCharge(DbmsCtx* ctx)
{
    InitialChargeBuffer buffer;
    buffer.q0 = (uint64_t)(ctx->qstats.initial * 1e6);
    buffer.set_at = ctx->qstats.initial_set_ts;
    return SaveStoredObject(ctx, EEPROM_INITIAL_CHARGE, &buffer, sizeof(buffer));
}
