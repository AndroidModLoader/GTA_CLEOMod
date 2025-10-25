#include <mod/amlmod.h>
#include <mod/logger.h>
#include <cleohelpers.h>

// ------------------------------------[1C00 - 1C09]---------------------------------------------

CLEO_Fn(DEGREE_TO_RADIAN)
{
    float degree = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = degree * (M_PI / 180.0);
}
CLEO_Fn(RADIAN_TO_DEGREE)
{
    float radian = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = radian * (180.0 / M_PI);
}
CLEO_Fn(INT_MODULO)
{
    int x = cleo->ReadParam(handle)->i;
    int y = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = x % y;
}
CLEO_Fn(FLOAT_MODULO)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = fmod(x, y);
}
CLEO_Fn(ACOS)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = acos(f);
}
CLEO_Fn(ASIN)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = asin(f);
}
CLEO_Fn(ATAN)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = atan(f);
}
CLEO_Fn(CUBEROOT)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = cbrt(f);
}
CLEO_Fn(CEIL)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = ceil(f);
}
CLEO_Fn(COS)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = cos(f);
}

// ------------------------------------[1C10 - 1C19]---------------------------------------------

CLEO_Fn(COSH)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = cosh(f);
}
CLEO_Fn(EXPM1)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = expm1(f);
}
CLEO_Fn(FDIM)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = fdim(x, y);
}
CLEO_Fn(FLOOR)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = floor(f);
}
CLEO_Fn(HYPOT)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = hypot(x, y);
}
CLEO_Fn(FMA)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    float z = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = fma(x, y, z);
}
CLEO_Fn(FMAX)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = fmax(x, y);
}
CLEO_Fn(FMIN)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = fmin(x, y);
}
CLEO_Fn(SIN)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = sin(f);
}
CLEO_Fn(SINH)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = sinh(f);
}

// ------------------------------------[1C20 - 1C29]---------------------------------------------

CLEO_Fn(TAN)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = tan(f);
}
CLEO_Fn(TANH)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = tanh(f);
}
CLEO_Fn(ATAN2)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = atan2(x, y);
}
CLEO_Fn(FREXP)
{
    float f = cleo->ReadParam(handle)->f;
    int expVar = 0;
    cleo->GetPointerToScriptVar(handle)->f = frexp(f, &expVar);
    cleo->GetPointerToScriptVar(handle)->i = expVar;
}
CLEO_Fn(LDEXP)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = ldexp(x, y);
}
CLEO_Fn(MODF)
{
    float x = cleo->ReadParam(handle)->f;
    float y = 0.0f;
    cleo->GetPointerToScriptVar(handle)->f = modf(x, &y);
    cleo->GetPointerToScriptVar(handle)->f = y;
}
CLEO_Fn(SCALBN)
{
    float x = cleo->ReadParam(handle)->f;
    int n = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->f = scalbn(x, n);
}
CLEO_Fn(TRUNC)
{
    float x = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = trunc(x);
}
CLEO_Fn(REMAINDER)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = remainder(x, y);
}
CLEO_Fn(FPCLASSIFY)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->i = fpclassify(f);
}

// ------------------------------------[1C30 - 1C39]---------------------------------------------

CLEO_Fn(CLAMP_FLOAT)
{
    float f = cleo->ReadParam(handle)->f;
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    if(f < a) f = a;
    else if(f > b) f = b;
    cleo->GetPointerToScriptVar(handle)->f = f;
}
CLEO_Fn(CLAMP_INT)
{
    int f = cleo->ReadParam(handle)->i;
    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    if(f < a) f = a;
    else if(f > b) f = b;
    cleo->GetPointerToScriptVar(handle)->i = f;
}
CLEO_Fn(DISTANCE_FLOAT)
{
    GTAVector3D a, b;

    a.x = cleo->ReadParam(handle)->f;
    a.y = cleo->ReadParam(handle)->f;
    a.z = cleo->ReadParam(handle)->f;

    b.x = cleo->ReadParam(handle)->f;
    b.y = cleo->ReadParam(handle)->f;
    b.z = cleo->ReadParam(handle)->f;

    cleo->GetPointerToScriptVar(handle)->f = a.GetDistance(&b);
}
CLEO_Fn(DISTANCE_VECTOR)
{
    GTAVector3D *a, *b;

    a = (GTAVector3D*)cleo->ReadParam(handle)->i;
    b = (GTAVector3D*)cleo->ReadParam(handle)->i;

    cleo->GetPointerToScriptVar(handle)->f = a->GetDistance(b);
}
CLEO_Fn(DISTANCE2D_FLOAT)
{
    GTAVector3D a, b;

    a.x = cleo->ReadParam(handle)->f;
    a.y = cleo->ReadParam(handle)->f;

    b.x = cleo->ReadParam(handle)->f;
    b.y = cleo->ReadParam(handle)->f;

    cleo->GetPointerToScriptVar(handle)->f = a.GetDistance2D(&b);
}
CLEO_Fn(DISTANCE2D_VECTOR)
{
    GTAVector3D *a, *b;

    a = (GTAVector3D*)cleo->ReadParam(handle)->i;
    b = (GTAVector3D*)cleo->ReadParam(handle)->i;

    cleo->GetPointerToScriptVar(handle)->f = a->GetDistance2D(b);
}
CLEO_Fn(INVSQRT)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = 1.0 / sqrt(f);
}
CLEO_Fn(TGAMMA)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = tgammaf(f);
}
CLEO_Fn(LGAMMA)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = lgammaf(f);
}
CLEO_Fn(REMQUO)
{
    int quo = 0;
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = remquof(x, y, &quo);
    cleo->GetPointerToScriptVar(handle)->i = quo;
}

// ------------------------------------[1C40 - 1C49]---------------------------------------------

CLEO_Fn(EXP)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = expf(f);
}
CLEO_Fn(EXP2)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = exp2f(f);
}
CLEO_Fn(ERRORF)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = erf(f);
}
CLEO_Fn(ERRORFCOMPLEMENTARY)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = erfc(f);
}
CLEO_Fn(NEXTAFTER)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = nextafterf(x, y);
}
CLEO_Fn(NEXTTOWARD)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = nexttowardf(x, y);
}
CLEO_Fn(COPYSIGN)
{
    float x = cleo->ReadParam(handle)->f;
    float y = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = copysignf(x, y);
}
CLEO_Fn(TOGGLEBOOL)
{
    int* b = &cleo->GetPointerToScriptVar(handle)->i;
    *b = (*b == 0);
}
CLEO_Fn(MINMAXSHUFFLE)
{
    float& min = cleo->GetPointerToScriptVar(handle)->f;
    float& max = cleo->GetPointerToScriptVar(handle)->f;
    if(min < max)
    {
        UpdateCompareFlag(handle, false);
    }
    else
    {
        float shuffled = min;
        min = max;
        max = shuffled;
        UpdateCompareFlag(handle, true);
    }
}
CLEO_Fn(MINMAXSHUFFLE3)
{
    float& min = cleo->GetPointerToScriptVar(handle)->f;
    float& mid = cleo->GetPointerToScriptVar(handle)->f;
    float& max = cleo->GetPointerToScriptVar(handle)->f;
    if(min < mid && mid < max)
    {
        UpdateCompareFlag(handle, false);
    }
    else
    {
        float t;
        if (min > mid) { t = min; min = mid; mid = t; }
        if (mid > max) { t = mid; mid = max; max = t; }
        if (min > mid) { t = min; min = mid; mid = t; }
        UpdateCompareFlag(handle, true);
    }
}

// ------------------------------------[1C50 - 1C59]---------------------------------------------

CLEO_Fn(LOGB)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = logbf(f);
}
CLEO_Fn(ILOGB)
{
    float f = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->i = ilogbf(f);
}
CLEO_Fn(SIGNBIT)
{
    unsigned int f = cleo->ReadParam(handle)->u;
    cleo->GetPointerToScriptVar(handle)->i = (f >> 16) & 0x8000;
}
CLEO_Fn(LERP)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    float t = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = a + (b - a) * t;
}
CLEO_Fn(PYTHA)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = sqrtf(a * a + b * b);
}
CLEO_Fn(RULEOFTHREE)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    float c = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = (b * c) / a;
}
CLEO_Fn(UNLERP)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    float v = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = (v - a) / (b - a);
}
CLEO_Fn(SMOOTHSTEP)
{
    float edge1 = cleo->ReadParam(handle)->f;
    float edge2 = cleo->ReadParam(handle)->f;
    float x = cleo->ReadParam(handle)->f;
    x = clampfloat(0.0f, 1.0f, (x - edge1) / (edge2 - edge1)); // amlmod.h
    cleo->GetPointerToScriptVar(handle)->f = x * x * (3.0f - 2.0f * x);
}
CLEO_Fn(SMOOTHERSTEP)
{
    float edge1 = cleo->ReadParam(handle)->f;
    float edge2 = cleo->ReadParam(handle)->f;
    float x = cleo->ReadParam(handle)->f;
    x = clampfloat(0.0f, 1.0f, (x - edge1) / (edge2 - edge1)); // amlmod.h
    cleo->GetPointerToScriptVar(handle)->f = x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}
CLEO_Fn(INVSMOOTHSTEP)
{
    float y = clampfloat(0.0f, 1.0f, cleo->ReadParam(handle)->f);
    cleo->GetPointerToScriptVar(handle)->f = 0.5f - sinf(asinf(1.0f - 2.0f * y) / 3.0f);
}

// ------------------------------------[   ~Main   ]---------------------------------------------

void InitMathOpcodes()
{
    CLEO_RegisterOpcode(0x1C00, DEGREE_TO_RADIAN); // 1C00=2,%2d% = to_radian %1d%
    CLEO_RegisterOpcode(0x1C01, RADIAN_TO_DEGREE); // 1C01=2,%2d% = to_degree %1d%
    CLEO_RegisterOpcode(0x1C02, INT_MODULO); // 1C02=3,%3d% = modulo_int %1d% %2d%
    CLEO_RegisterOpcode(0x1C03, FLOAT_MODULO); // 1C03=3,%3d% = modulo_float %1d% %2d%
    CLEO_RegisterOpcode(0x1C04, ACOS); // 1C04=2,%2d% = acos %1d%
    CLEO_RegisterOpcode(0x1C05, ASIN); // 1C05=2,%2d% = asin %1d%
    CLEO_RegisterOpcode(0x1C06, ATAN); // 1C06=2,%2d% = atan %1d%
    CLEO_RegisterOpcode(0x1C07, CUBEROOT); // 1C07=2,%2d% = cbrt %1d%
    CLEO_RegisterOpcode(0x1C08, CEIL); // 1C08=2,%2d% = ceil %1d%
    CLEO_RegisterOpcode(0x1C09, COS); // 1C09=2,%2d% = cos %1d%

    CLEO_RegisterOpcode(0x1C10, COSH); // 1C10=2,%2d% = cosh %1d%
    CLEO_RegisterOpcode(0x1C11, EXPM1); // 1C11=2,%2d% = expm1 %1d%
    CLEO_RegisterOpcode(0x1C12, FDIM); // 1C12=3,%3d% = fdim %1d% %2d%
    CLEO_RegisterOpcode(0x1C13, FLOOR); // 1C13=2,%2d% = floor %1d%
    CLEO_RegisterOpcode(0x1C14, HYPOT); // 1C14=3,%3d% = hypot %1d% %2d%
    CLEO_RegisterOpcode(0x1C15, FMA); // 1C15=4,%4d% = fma %1d% %2d% %3d%
    CLEO_RegisterOpcode(0x1C16, FMAX); // 1C16=3,%3d% = fmax %1d% %2d%
    CLEO_RegisterOpcode(0x1C17, FMIN); // 1C17=3,%3d% = fmin %1d% %2d%
    CLEO_RegisterOpcode(0x1C18, SIN); // 1C18=2,%2d% = sin %1d%
    CLEO_RegisterOpcode(0x1C19, SINH); // 1C19=2,%2d% = sinh %1d%

    CLEO_RegisterOpcode(0x1C20, TAN); // 1C20=2,%2d% = tan %1d%
    CLEO_RegisterOpcode(0x1C21, TANH); // 1C21=2,%2d% = tanh %1d%
    CLEO_RegisterOpcode(0x1C22, ATAN2); // 1C22=3,%3d% = atan2 %1d% %2d%
    CLEO_RegisterOpcode(0x1C23, FREXP); // 1C23=3,%2d% exp %3d% = frexp %1d%
    CLEO_RegisterOpcode(0x1C24, LDEXP); // 1C24=3,%3d% = ldexp %1d% exp %2d%
    CLEO_RegisterOpcode(0x1C25, MODF); // 1C25=3,%2d% intpart %3d% = modf %1d%
    CLEO_RegisterOpcode(0x1C26, SCALBN); // 1C26=3,%3d% = scalbn %1d% int_n %2d%
    CLEO_RegisterOpcode(0x1C27, TRUNC); // 1C27=2,%2d% = trunc %1d%
    CLEO_RegisterOpcode(0x1C28, REMAINDER); // 1C28=3,%3d% = remainder %1d% %2d%
    CLEO_RegisterOpcode(0x1C29, FPCLASSIFY); // 1C29=2,%2d% = fpclassify %1d%
    
    CLEO_RegisterOpcode(0x1C30, CLAMP_FLOAT); // 1C30=4,%4d% = clamp_float %1d% limit %2d% %3d%
    CLEO_RegisterOpcode(0x1C31, CLAMP_INT); // 1C31=4,%4d% = clamp_int %1d% limit %2d% %3d%
    CLEO_RegisterOpcode(0x1C32, DISTANCE_FLOAT); // 1C32=7,%7d% = distance_from %1d% %2d% %3d% to %4d% %5d% %6d%
    CLEO_RegisterOpcode(0x1C33, DISTANCE_VECTOR); // 1C33=3,%3d% = distance_from %1d% to_vec %2d%
    CLEO_RegisterOpcode(0x1C34, DISTANCE2D_FLOAT); // 1C34=5,%5d% = distance2d_from %1d% %2d% to %3d% %4d%
    CLEO_RegisterOpcode(0x1C35, DISTANCE2D_VECTOR); // 1C35=3,%3d% = distance2d_from %1d% to_vec %2d%
    CLEO_RegisterOpcode(0x1C36, INVSQRT); // 1C36=2,%2d% = invsqrt %1d%
    CLEO_RegisterOpcode(0x1C37, TGAMMA); // 1C37=2,%2d% = tgamma %1d%
    CLEO_RegisterOpcode(0x1C38, LGAMMA); // 1C38=2,%2d% = lgamma %1d%
    CLEO_RegisterOpcode(0x1C39, REMQUO); // 1C39=4,%3d% quo %4d% = remquo %1d% %2d%
    
    CLEO_RegisterOpcode(0x1C40, EXP); // 1C40=2,%2d% = exp %1d%
    CLEO_RegisterOpcode(0x1C41, EXP2); // 1C41=2,%2d% = exp2 %1d%
    CLEO_RegisterOpcode(0x1C42, ERRORF); // 1C42=2,%2d% = erf %1d%
    CLEO_RegisterOpcode(0x1C43, ERRORFCOMPLEMENTARY); // 1C43=2,%2d% = erfc %1d%
    CLEO_RegisterOpcode(0x1C44, NEXTAFTER); // 1C44=3,%3d% = nextafter_from %1d% to %2d%
    CLEO_RegisterOpcode(0x1C45, NEXTTOWARD); // 1C45=3,%3d% = nexttoward_from %1d% to %2d%
    CLEO_RegisterOpcode(0x1C46, COPYSIGN); // 1C46=3,%3d% = copysign %1d% %2d%
    CLEO_RegisterOpcode(0x1C47, TOGGLEBOOL); // 1C47=1,toggle_bool %1d%
    CLEO_RegisterOpcode(0x1C48, MINMAXSHUFFLE); // 1C48=2,minmax_shuffle %1d% %2d% //IF and SET
    CLEO_RegisterOpcode(0x1C49, MINMAXSHUFFLE3); // 1C49=3,minmax_shuffle3 %1d% %2d% %3d% //IF and SET
    
    CLEO_RegisterOpcode(0x1C50, LOGB); // 1C50=2,%2d% = logb %1d%
    CLEO_RegisterOpcode(0x1C51, ILOGB); // 1C51=2,%2d% = ilogb %1d%
    CLEO_RegisterOpcode(0x1C52, SIGNBIT); // 1C52=2,%2d% = signbit %1d%
    CLEO_RegisterOpcode(0x1C53, LERP); // 1C53=4,%4d% = lerp %1d% %2d% t %3d%
    CLEO_RegisterOpcode(0x1C54, PYTHA); // 1C54=3,%3d% = pytha %1d% %2d%
    CLEO_RegisterOpcode(0x1C55, RULEOFTHREE); // 1C55=4,%4d% = r_of_t %1d% %2d% %3d%
    CLEO_RegisterOpcode(0x1C56, UNLERP); // 1C56=4,%4d% = unlerp %1d% %2d% v %3d%
    CLEO_RegisterOpcode(0x1C57, SMOOTHSTEP); // 1C57=4,%4d% = smoothstep %1d% %2d% x %3d%
    CLEO_RegisterOpcode(0x1C58, SMOOTHERSTEP); // 1C58=4,%4d% = smootherstep %1d% %2d% x %3d%
    CLEO_RegisterOpcode(0x1C59, INVSMOOTHSTEP); // 1C59=2,%2d% = inv_smoothstep %1d%
}