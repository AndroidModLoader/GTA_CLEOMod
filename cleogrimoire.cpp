#include <mod/amlmod.h>
#include <mod/logger.h>
#include <cleohelpers.h>

#include <dirent.h>
#include <list>
#include <string>
#include <sys/stat.h>

#include <math.h>
#include <cstdint>


int (*TouchInterface_PositionWidgets)();

/////////////////////////////////////////////////////
/////////// BEGIN OPCODES by MatiDragon /////////////
/////////////////////////////////////////////////////

// Helpers clamp
static float clampf(float v, float a, float b) { if (v < a) return a; if (v > b) return b; return v; }
static int clampi(int v, int a, int b) { if (v < a) return a; if (v > b) return b; return v; }

static uintptr_t g_widgetsBase = 0;
static inline float* GetWidgetProps(int widgetId)
{
    // cachea una sola vez
    if (!g_widgetsBase)
        g_widgetsBase = (uintptr_t)TouchInterface_PositionWidgets;

    // *(g_widgetsBase + id*4) → puntero al widget
    uintptr_t widgetPtr = *(uintptr_t*)(g_widgetsBase + (widgetId << 2));

    // offset 12 → props
    return (float*)(widgetPtr + 12);
}

CLEO_Fn(SET_WIDGET_TRANSFORM)
{
    int id = cleo->ReadParam(handle)->i;
    float* p = GetWidgetProps(id);

    p[0] = cleo->ReadParam(handle)->f;
    p[1] = cleo->ReadParam(handle)->f;
    p[2] = cleo->ReadParam(handle)->f;
    p[3] = cleo->ReadParam(handle)->f;
}

CLEO_Fn(GET_WIDGET_TRANSFORM)
{
    int id = cleo->ReadParam(handle)->i;
    float* p = GetWidgetProps(id);

    auto out = cleo->GetPointerToScriptVar(handle);
    out[0].f = p[0];
    out[1].f = p[1];
    out[2].f = p[2];
    out[3].f = p[3];
}

CLEO_Fn(CREATE_FILE_OR_DIRECTORY)
{
    char filepath[256];
    CLEO_ReadStringEx(handle, filepath, sizeof(filepath));

    // Validar que la ruta no esté vacía
    if (strlen(filepath) == 0)
    {
        UpdateCompareFlag(handle, false);
        return;
    }

    // Resolver ruta
    std::string path = ResolvePath(handle, filepath);

    // Verificar si ya existe
    struct stat info;
    if (stat(path.c_str(), &info) == 0)
    {
        UpdateCompareFlag(handle, true); // Ya existe
        return;
    }

    // Crear archivo o carpeta
    if (filepath[strlen(filepath) - 1] == '/') // Si termina en '/', es una carpeta
    {
        int result = mkdir(path.c_str(), 0777);
        UpdateCompareFlag(handle, result == 0);
    }
    else // Si no, es un archivo
    {
        FILE* file = fopen(path.c_str(), "w");
        if (file)
        {
            fclose(file);
            UpdateCompareFlag(handle, true);
        }
        else
        {
            UpdateCompareFlag(handle, false);
        }
    }
}

CLEO_Fn(ANGLE_DIFF)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;

    float diff = fmodf(b - a + 180.0f, 360.0f);
    if (diff < 0.0f) diff += 360.0f;
    diff -= 180.0f;

    cleo->GetPointerToScriptVar(handle)->f = diff;
}

CLEO_Fn(TOGGLE_BOOLEAN_VAR)
{
    int v = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = (v == 0) ? 1 : 0;
}

CLEO_Fn(TOGGLE_BOOLEAN_REAL)
{
    int v = cleo->ReadParam(handle)->i;
    cleo->GetPointerToScriptVar(handle)->i = (v == 0) ? 0 : 1;
}

CLEO_Fn(FLOAT_DIV)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = a / b;
}

CLEO_Fn(FLOAT_MUL)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = a * b;
}

CLEO_Fn(FLOAT_SUM)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = a + b;
}

CLEO_Fn(FLOAT_SUB)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    cleo->GetPointerToScriptVar(handle)->f = a - b;
}

CLEO_Fn(SPLIT_FLOAT_TO_SIGNED_PARTS)
{
    float v = cleo->ReadParam(handle)->f;
    int decimals = clampi(cleo->ReadParam(handle)->i, 0, 9);

    int sign = (v < 0.0f) ? -1 : 1;
    float absv = fabsf(v);

    int intPart = (int)absv * sign;

    int fracPart = 0;
    if (decimals > 0)
    {
        float scaled = (absv - (float)((int)absv)) * powf(10.0f, decimals);
        fracPart = (int)scaled * sign;
    }

    auto out = cleo->GetPointerToScriptVar(handle);
    out[0].i = intPart;
    out[1].i = fracPart;
}

CLEO_Fn(FILE_RENAME)
{
    char srcPathBuf[256];
    char dstPathBuf[256];
    CLEO_ReadStringEx(handle, srcPathBuf, sizeof(srcPathBuf));
    CLEO_ReadStringEx(handle, dstPathBuf, sizeof(dstPathBuf));
    srcPathBuf[sizeof(srcPathBuf)-1] = 0;
    dstPathBuf[sizeof(dstPathBuf)-1] = 0;

    // normalizar slashes
    for (int i = 0; srcPathBuf[i]; ++i) if (srcPathBuf[i] == '\\') srcPathBuf[i] = '/';
    for (int i = 0; dstPathBuf[i]; ++i) if (dstPathBuf[i] == '\\') dstPathBuf[i] = '/';

    std::string src = ResolvePath(handle, srcPathBuf);
    std::string dst = ResolvePath(handle, dstPathBuf);

    // Intentar rename directo (mover/renombrar)
    int res = rename(src.c_str(), dst.c_str());
    if (res == 0)
    {
        UpdateCompareFlag(handle, true);
        return;
    }

    // Si falla el rename, intentar fallback copy + remove (útil entre dispositivos/FS distintos)
    FILE* fin = fopen(src.c_str(), "rb");
    if (!fin)
    {
        UpdateCompareFlag(handle, false);
        return;
    }
    FILE* fout = fopen(dst.c_str(), "wb");
    if (!fout)
    {
        fclose(fin);
        UpdateCompareFlag(handle, false);
        return;
    }

    const size_t BUF_SIZE = 8192;
    char *buffer = (char*)malloc(BUF_SIZE);
    if (!buffer)
    {
        fclose(fin);
        fclose(fout);
        UpdateCompareFlag(handle, false);
        return;
    }

    bool ok = true;
    size_t n;
    while ((n = fread(buffer, 1, BUF_SIZE, fin)) > 0)
    {
        if (fwrite(buffer, 1, n, fout) != n)
        {
            ok = false;
            break;
        }
    }

    free(buffer);
    fclose(fin);
    fflush(fout);
    fclose(fout);

    if (!ok)
    {
        // intento fallido: borrar destino parcial
        remove(dst.c_str());
        UpdateCompareFlag(handle, false);
        return;
    }

    // borrar origen original si la copia fue exitosa
    if (remove(src.c_str()) != 0)
    {
        // copia exitosa pero no se pudo borrar origen => consideramos fallo
        // opcional: podríamos retornar true y dejar origen; aquí devolvemos false
        UpdateCompareFlag(handle, false);
        return;
    }

    UpdateCompareFlag(handle, true);
}

// --- helpers: conversions (simple, comentadas) ------------------------

// RGB(0..255) -> HSV(H:0..360 int, S:0..100 int, V:0..100 int)
static void RGB_to_HSV(int r, int g, int b, int &outH, int &outS, int &outV)
{
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float maxv = rf; if (gf > maxv) maxv = gf; if (bf > maxv) maxv = bf;
    float minv = rf; if (gf < minv) minv = gf; if (bf < minv) minv = bf;
    float delta = maxv - minv;

    float h = 0.0f;
    if (delta <= 1e-6f) h = 0.0f;
    else if (maxv == rf) h = 60.0f * fmodf(((gf - bf) / delta), 6.0f);
    else if (maxv == gf) h = 60.0f * (((bf - rf) / delta) + 2.0f);
    else h = 60.0f * (((rf - gf) / delta) + 4.0f);
    if (h < 0.0f) h += 360.0f;

    float s = (maxv <= 1e-6f) ? 0.0f : (delta / maxv);
    float v = maxv;

    outH = clampi((int)roundf(h), 0, 360);
    outS = clampi((int)roundf(s * 100.0f), 0, 100);
    outV = clampi((int)roundf(v * 100.0f), 0, 100);
}

// HSV(H:0..360, S:0..100, V:0..100) -> RGB(0..255)
static void HSV_to_RGB(int H, int S, int V, int &outR, int &outG, int &outB)
{
    float h = (float)H;
    float s = (float)S / 100.0f;
    float v = (float)V / 100.0f;

    if (s <= 0.0f) {
        int val = clampi((int)roundf(v * 255.0f), 0, 255);
        outR = outG = outB = val;
        return;
    }

    float hh = fmodf(h, 360.0f) / 60.0f;
    if (hh < 0.0f) hh += 6.0f;
    int i = (int)floorf(hh);
    float f = hh - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    float rf=0, gf=0, bf=0;
    switch(i) {
        case 0: rf = v; gf = t; bf = p; break;
        case 1: rf = q; gf = v; bf = p; break;
        case 2: rf = p; gf = v; bf = t; break;
        case 3: rf = p; gf = q; bf = v; break;
        case 4: rf = t; gf = p; bf = v; break;
        default: rf = v; gf = p; bf = q; break;
    }

    outR = clampi((int)roundf(rf * 255.0f), 0, 255);
    outG = clampi((int)roundf(gf * 255.0f), 0, 255);
    outB = clampi((int)roundf(bf * 255.0f), 0, 255);
}

// RGB -> HSL (H:0..360, S:0..100, L:0..100)
static void RGB_to_HSL(int r, int g, int b, int &outH, int &outS, int &outL)
{
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float maxv = rf; if (gf > maxv) maxv = gf; if (bf > maxv) maxv = bf;
    float minv = rf; if (gf < minv) minv = gf; if (bf < minv) minv = bf;
    float l = (maxv + minv) * 0.5f;
    float delta = maxv - minv;

    float h = 0.0f;
    float s = 0.0f;

    if (delta <= 1e-6f) {
        h = 0.0f; s = 0.0f;
    } else {
        if (l < 0.5f) s = delta / (maxv + minv);
        else s = delta / (2.0f - maxv - minv);

        if (maxv == rf) h = 60.0f * fmodf(((gf - bf) / delta), 6.0f);
        else if (maxv == gf) h = 60.0f * (((bf - rf) / delta) + 2.0f);
        else h = 60.0f * (((rf - gf) / delta) + 4.0f);
        if (h < 0.0f) h += 360.0f;
    }

    outH = clampi((int)roundf(h), 0, 360);
    outS = clampi((int)roundf(s * 100.0f), 0, 100);
    outL = clampi((int)roundf(l * 100.0f), 0, 100);
}

// HSL -> RGB
static void HSL_to_RGB(int H, int S, int L, int &outR, int &outG, int &outB)
{
    float h = fmodf((float)H, 360.0f);
    if (h < 0.0f) h += 360.0f;
    float s = (float)S / 100.0f;
    float l = (float)L / 100.0f;

    float rf, gf, bf;
    if (s <= 0.0f) {
        rf = gf = bf = l;
    } else {
        float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
        float p = 2.0f * l - q;
        float hk = h / 360.0f;
        auto hue_to_rgb_local = [](float p, float q, float t)->float {
            if (t < 0.0f) t += 1.0f;
            if (t > 1.0f) t -= 1.0f;
            if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
            if (t < 1.0f/2.0f) return q;
            if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
            return p;
        };
        rf = hue_to_rgb_local(p, q, hk + 1.0f/3.0f);
        gf = hue_to_rgb_local(p, q, hk);
        bf = hue_to_rgb_local(p, q, hk - 1.0f/3.0f);
    }

    outR = clampi((int)roundf(rf * 255.0f), 0, 255);
    outG = clampi((int)roundf(gf * 255.0f), 0, 255);
    outB = clampi((int)roundf(bf * 255.0f), 0, 255);
}

// HSV (H:0..360, S:0..100, V:0..100) → HSL (H:0..360, S:0..100, L:0..100)
static void HSV_to_HSL(int H, int S, int V, int &outH, int &outS, int &outL)
{
    // Normalizar hue como en el resto de funciones
    float hh = fmodf((float)H, 360.0f);
    if (hh < 0.0f) hh += 360.0f;
    outH = clampi((int)roundf(hh), 0, 360);

    float s = clampf((float)S / 100.0f, 0.0f, 1.0f);
    float v = clampf((float)V / 100.0f, 0.0f, 1.0f);

    float l  = v * (1.0f - 0.5f * s);
    float sl = 0.0f;
    if (l > 0.0f && l < 1.0f)
        sl = (v - l) / fminf(l, 1.0f - l);   // equivale a chroma / (2 * min(l,1-l))

    outL = clampi((int)roundf(l  * 100.0f), 0, 100);
    outS = clampi((int)roundf(sl * 100.0f), 0, 100);
}

// HSL (H:0..360, S:0..100, L:0..100) → HSV (H:0..360, S:0..100, V:0..100)
static void HSL_to_HSV(int H, int S, int L, int &outH, int &outS, int &outV)
{
    // Normalizar hue igual que arriba
    float hh = fmodf((float)H, 360.0f);
    if (hh < 0.0f) hh += 360.0f;
    outH = clampi((int)roundf(hh), 0, 360);

    float sl = clampf((float)S / 100.0f, 0.0f, 1.0f);
    float l  = clampf((float)L / 100.0f, 0.0f, 1.0f);

    float chroma = sl * (1.0f - fabsf(2.0f * l - 1.0f));
    float v      = l + 0.5f * chroma;
    float sv     = (v < 1e-6f) ? 0.0f : chroma / v;

    outV = clampi((int)roundf(v  * 100.0f), 0, 100);
    outS = clampi((int)roundf(sv * 100.0f), 0, 100);
}

enum CONVERT_MODE {
  RGB_TO_HSV,
  RGB_TO_HSL,
  HSL_TO_HSV,
  HSL_TO_RGB,
  HSV_TO_HSL,
  HSV_TO_RGB
};

// [a, b, c] = CONVERT_MODEL_COLOR(mode, x, y, z)
CLEO_Fn(CONVERT_MODEL_COLOR)
{
    int mode = cleo->ReadParam(handle)->i; // convertion mode

    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    int c = cleo->ReadParam(handle)->i;

    int X,Y,Z;
    switch (mode)
    {
    case CONVERT_MODE::RGB_TO_HSV:
        RGB_to_HSV(a,b,c,X,Y,Z);
        break;
    case CONVERT_MODE::RGB_TO_HSL:
        RGB_to_HSL(a,b,c,X,Y,Z);
        break;
    case CONVERT_MODE::HSL_TO_HSV:
        HSL_to_HSV(a,b,c,X,Y,Z);
        break;
    case CONVERT_MODE::HSL_TO_RGB:
        HSL_to_RGB(a,b,c,X,Y,Z);
        break;
    case CONVERT_MODE::HSV_TO_HSL:
        HSV_to_HSL(a,b,c,X,Y,Z);
        break;
    case CONVERT_MODE::HSV_TO_RGB:
        HSV_to_RGB(a,b,c,X,Y,Z);
        break;
    default:
        X = Y = Z = 0;
        break;
    }

    cleo->GetPointerToScriptVar(handle)->i = X;
    cleo->GetPointerToScriptVar(handle)->i = Y;
    cleo->GetPointerToScriptVar(handle)->i = Z;
}

// op: 0==, 1!=, 2<, 3<=, 4>, 5>=
static bool CompareInts(int a, int b, int op)
{
    switch(op)
    {
        case 0: return a == b;
        case 1: return a != b;
        case 2: return a <  b;
        case 3: return a <= b;
        case 4: return a >  b;
        case 5: return a >= b;
        default: return false;
    }
}
static bool CompareFloats(float a, float b, int op)
{
    switch(op)
    {
        case 0: return a == b;
        case 1: return a != b;
        case 2: return a <  b;
        case 3: return a <= b;
        case 4: return a >  b;
        case 5: return a >= b;
        default: return false;
    }
}

// out = LOGICAL_OR a b
CLEO_Fn(LOGICAL_OR)
{
    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;

    cleo->GetPointerToScriptVar(handle)->i = (a != 0) ? a : b;
}


//%4d% = %1d% ? %2d% : %3d%
CLEO_Fn(IF_TERNARY)
{
    int a = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    int c = cleo->ReadParam(handle)->i;

    cleo->GetPointerToScriptVar(handle)->i = (a != 0) ? b : c;
}

// out = IF_TERNARY_INT A op B ? C : D
CLEO_Fn(IF_TERNARY_INT)
{
    int A  = cleo->ReadParam(handle)->i;
    int op = cleo->ReadParam(handle)->i;
    int B  = cleo->ReadParam(handle)->i;
    std::uint32_t C  = cleo->ReadParam(handle)->i;
    std::uint32_t D  = cleo->ReadParam(handle)->i;

    std::uint32_t result = CompareInts(A, B, op) ? C : D;

    cleo->GetPointerToScriptVar(handle)->i = result;
}

// out = IF_TERNARY_FLOAT A op B ? C : D
CLEO_Fn(IF_TERNARY_FLOAT)
{
    float A  = cleo->ReadParam(handle)->f;
    int op = cleo->ReadParam(handle)->i;
    float B  = cleo->ReadParam(handle)->f;
    std::uint32_t C  = cleo->ReadParam(handle)->f;
    std::uint32_t D  = cleo->ReadParam(handle)->f;

    std::uint32_t result = CompareFloats(A, B, op) ? C : D;

    cleo->GetPointerToScriptVar(handle)->i = result;
}



// ----------------------------------------------------------------


// ===================================================================
// PACK / UNPACK cuatro valores 8-bit (signed o unsigned según flags) → int32
// Flags como int binario directo, sin #define:
// bit 0 (1)      → byte0 (lowest) signed
// bit 1 (2)      → byte1 signed  
// bit 2 (4)      → byte2 signed
// bit 3 (8)      → byte3 (highest) signed
// Ejemplos:
// 0b0000 → todos unsigned 0..255
// 0b1111 → todos signed   -128..127
// 0b0011 → solo byte0 y byte1 signed
// ===================================================================

static void PackFourInt8_to_Int32(int b3, int b2, int b1, int b0, int flags, int &out)
{
    auto clampb = [&](int v, int f) {
        return (f ? clampi(v, -128, 127) : clampi(v, 0, 255));
    };

    unsigned char u3 = (unsigned char)clampb(b3, flags & 8);
    unsigned char u2 = (unsigned char)clampb(b2, flags & 4);
    unsigned char u1 = (unsigned char)clampb(b1, flags & 2);
    unsigned char u0 = (unsigned char)clampb(b0, flags & 1);

    out = (u3 << 24) | (u2 << 16) | (u1 << 8) | u0;
}


static void UnpackInt32_to_FourInt8(int packed, int flags, int &outByte3, int &outByte2, int &outByte1, int &outByte0)
{
    unsigned char u3 = (packed >> 24) & 0xFF;
    unsigned char u2 = (packed >> 16) & 0xFF;
    unsigned char u1 = (packed >>  8) & 0xFF;
    unsigned char u0 =  packed        & 0xFF;

    outByte3 = (flags & (1<<3)) ? static_cast<signed char>(u3) : int(u3);
    outByte2 = (flags & (1<<2)) ? static_cast<signed char>(u2) : int(u2);
    outByte1 = (flags & (1<<1)) ? static_cast<signed char>(u1) : int(u1);
    outByte0 = (flags & (1<<0)) ? static_cast<signed char>(u0) : int(u0);
}
// packed = PACK_4DEC_TO_INT32 byte3 byte2 byte1 byte0 flags
CLEO_Fn(PACK_4DEC_TO_INT32)
{
    int byte3 = cleo->ReadParam(handle)->i;
    int byte2 = cleo->ReadParam(handle)->i;
    int byte1 = cleo->ReadParam(handle)->i;
    int byte0 = cleo->ReadParam(handle)->i;
    int flags = cleo->ReadParam(handle)->i;   // 5 parámetros de entrada

    int packed;
    PackFourInt8_to_Int32(byte3, byte2, byte1, byte0, flags, packed);

    cleo->GetPointerToScriptVar(handle)->i = packed;   // 0@ = valor empaquetado
}

// byte3 byte2 byte1 byte0 = UNPACK_INT32_TO_4DEC packed flags
CLEO_Fn(UNPACK_INT32_TO_4DEC)
{
    int packed = cleo->ReadParam(handle)->i;
    int flags  = cleo->ReadParam(handle)->i;

    int byte3, byte2, byte1, byte0;
    UnpackInt32_to_FourInt8(packed, flags, byte3, byte2, byte1, byte0);

    cleo->GetPointerToScriptVar(handle)->i = byte3;
    cleo->GetPointerToScriptVar(handle)->i = byte2;
    cleo->GetPointerToScriptVar(handle)->i = byte1;
    cleo->GetPointerToScriptVar(handle)->i = byte0;
}

static int PackSetByte_Internal(int packed, int byteIndex, int value, int isSigned)
{
    int clamped = isSigned ? clampi(value, -128, 127) : clampi(value, 0, 255);
    unsigned char u = static_cast<unsigned char>(clamped);

    int shift = byteIndex * 8;
    packed &= ~(0xFF << shift);
    packed |= (int(u) << shift);

    return packed;
}

// packed = PACK_SET_BYTE packed byteIndex newValue isSigned
CLEO_Fn(PACK_SET_BYTE)
{
    int packed     = cleo->ReadParam(handle)->i;
    int byteIndex  = cleo->ReadParam(handle)->i;
    int newValue   = cleo->ReadParam(handle)->i;
    int isSigned   = cleo->ReadParam(handle)->i;

    cleo->GetPointerToScriptVar(handle)->i =
        PackSetByte_Internal(packed, byteIndex, newValue, isSigned);
}

static int PackGetByte_Internal(int packed, int byteIndex, int isSigned)
{
    int shift = byteIndex * 8;
    unsigned char u = (packed >> shift) & 0xFF;
    return isSigned ? int(static_cast<signed char>(u)) : int(u);
}

// byte = PACK_GET_BYTE packed byteIndex isSigned
CLEO_Fn(PACK_GET_BYTE)
{
    int packed    = cleo->ReadParam(handle)->i;
    int byteIndex = cleo->ReadParam(handle)->i;
    int isSigned  = cleo->ReadParam(handle)->i;

    cleo->GetPointerToScriptVar(handle)->i =
        PackGetByte_Internal(packed, byteIndex, isSigned);
}

static int PackRotateLeft32(int packed, int amount)
{
    amount &= 31;
    unsigned int u = (unsigned int)packed;
    return (u << amount) | (u >> (32 - amount));
}

static int PackRotateRight32(int packed, int amount)
{
    amount &= 31;
    unsigned int u = (unsigned int)packed;
    return (u >> amount) | (u << (32 - amount));
}

// result = PACK_ROTATE packed direction amount
CLEO_Fn(PACK_ROTATE)
{
    int packed = cleo->ReadParam(handle)->i;
    int direction = cleo->ReadParam(handle)->i;
    int amount = cleo->ReadParam(handle)->i;

    if (direction == 0) {
        cleo->GetPointerToScriptVar(handle)->i = PackRotateLeft32(packed, amount);
    }
    else {
        cleo->GetPointerToScriptVar(handle)->i = PackRotateRight32(packed, amount);
    }
}

static int PackCheckTruthy_Internal(int packed, int mask, int mode)
{
    unsigned char b3 = (packed >> 24) & 0xFF;
    unsigned char b2 = (packed >> 16) & 0xFF;
    unsigned char b1 = (packed >>  8) & 0xFF;
    unsigned char b0 =  packed        & 0xFF;

    bool arr[4] = { b0 != 0, b1 != 0, b2 != 0, b3 != 0 };

    if (mode == 0)  // ALL
    {
        for (int i = 0; i < 4; i++)
            if ((mask & (1<<i)) && !arr[i])
                return 0;
        return 1;
    }
    else            // ANY
    {
        for (int i = 0; i < 4; i++)
            if ((mask & (1<<i)) && arr[i])
                return 1;
        return 0;
    }
}
// truth = PACK_CHECK_TRUTHY packed mask mode
// mask bits: 1=byte0, 2=byte1, 4=byte2, 8=byte3
// mode: 0=ALL, 1=ANY
CLEO_Fn(PACK_CHECK_TRUTHY)
{
    int packed = cleo->ReadParam(handle)->i;
    int mask   = cleo->ReadParam(handle)->i;
    int mode   = cleo->ReadParam(handle)->i;

    cleo->GetPointerToScriptVar(handle)->i =
        PackCheckTruthy_Internal(packed, mask, mode);
}

static int PackSwapCustom(int p, int i3, int i2, int i1, int i0)
{
    unsigned char b[4];
    b[3] = (p >> 24) & 0xFF;
    b[2] = (p >> 16) & 0xFF;
    b[1] = (p >>  8) & 0xFF;
    b[0] =  p        & 0xFF;

    return (int(b[i3]) << 24) |
           (int(b[i2]) << 16) |
           (int(b[i1]) <<  8) |
           int(b[i0]);
}
// out = PACK_SWAP_CUSTOM packed i3 i2 i1 i0
CLEO_Fn(PACK_SWAP_CUSTOM)
{
    int p  = cleo->ReadParam(handle)->i;
    int i3 = cleo->ReadParam(handle)->i;
    int i2 = cleo->ReadParam(handle)->i;
    int i1 = cleo->ReadParam(handle)->i;
    int i0 = cleo->ReadParam(handle)->i;

    cleo->GetPointerToScriptVar(handle)->i = PackSwapCustom(p,i3,i2,i1,i0);
}



///////////////////////////////////////////////////
///////////////////// ORBITS //////////////////////
///////////////////////////////////////////////////


// Helpers (float)
static inline float DegToRadF(float deg) { return deg * (3.14159265358979323846f / 180.0f); }
static inline float LerpF(float a, float b, float t) { return a + (b - a) * t; }
static inline float Vec2Len(float x, float y) { return sqrtf(x*x + y*y); }

// 7017=7,%6d% %7d% = orbit_circle %1b:angle/radian% angle %2d% radius %3d% coords %4d% %5d%
CLEO_Fn(ORBIT_CIRCLE)
{
    int angleMode = cleo->ReadParam(handle)->i;
    float angle = cleo->ReadParam(handle)->f;
    float radius = cleo->ReadParam(handle)->f;
    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;

    if (angleMode == 0) angle = DegToRadF(angle);

    float x = cosf(angle) * radius + cx;
    float y = sinf(angle) * radius + cy;

    cleo->GetPointerToScriptVar(handle)->f = x;
    cleo->GetPointerToScriptVar(handle)->f = y;
}

// 7018=10,%8d% %9d% %10d% = orbit_sphere %1b:angle/radian% angles %2d% %3d% radius %4d% coords %5d% %6d% %7d%
CLEO_Fn(ORBIT_SPHERE)
{
    int angleMode = cleo->ReadParam(handle)->i;
    float ax = cleo->ReadParam(handle)->f;
    float ay = cleo->ReadParam(handle)->f;
    float radius = cleo->ReadParam(handle)->f;
    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;
    float cz = cleo->ReadParam(handle)->f;

    if (angleMode == 0) { ax = DegToRadF(ax); ay = DegToRadF(ay); }

    float sax = sinf(ax);
    float cax = cosf(ax);
    float say = sinf(ay);
    float cay = cosf(ay);

    float x = sax * cay * radius + cx;
    float y = sax * say * radius + cy;
    float z = cax * radius + cz;

    cleo->GetPointerToScriptVar(handle)->f = x;
    cleo->GetPointerToScriptVar(handle)->f = y;
    cleo->GetPointerToScriptVar(handle)->f = z;
}

// 7019=9,%8d% %9d% = orbit_oval %1b:angle/radian% angle %2d% radius %3d% %4d% rotation %5d% coords %6d% %7d%
CLEO_Fn(ORBIT_OVAL)
{
    int angleMode = cleo->ReadParam(handle)->i;
    float angle   = cleo->ReadParam(handle)->f;
    float radiusX = cleo->ReadParam(handle)->f;
    float radiusY = cleo->ReadParam(handle)->f;
    float rot2d   = cleo->ReadParam(handle)->f;
    float cx      = cleo->ReadParam(handle)->f;
    float cy      = cleo->ReadParam(handle)->f;

    // Convert angles to radians if needed
    if (angleMode == 0) {
        angle = DegToRadF(angle);
        rot2d = DegToRadF(rot2d);
    }

    // Parametric position on the ellipse (before rotation)
    // x' = cos(t) * a
    // y' = sin(t) * b
    float x = cosf(angle) * radiusX;
    float y = sinf(angle) * radiusY;

    // 👉 Optimización: si rot2d = 0, no rotar
    if (rot2d != 0.0f)
    {
        float s = sinf(rot2d);
        float c = cosf(rot2d);

        float xr = x * c - y * s;
        float yr = x * s + y * c;
        x = xr;
        y = yr;
    }

    cleo->GetPointerToScriptVar(handle)->f = x + cx;
    cleo->GetPointerToScriptVar(handle)->f = y + cy;
}

// 701A=15,%13d% %14d% %15d% = orbit_ovoid %1b:angle/radian% angles %2d% %3d% radius %4d% %5d% %6d% rotation %7d% %8d% %9d% coords %10d% %11d% %12d%
CLEO_Fn(ORBIT_OVOID)
{
    int angleMode = cleo->ReadParam(handle)->i;

    float ax = cleo->ReadParam(handle)->f;
    float ay = cleo->ReadParam(handle)->f;

    float radiusX = cleo->ReadParam(handle)->f;
    float radiusY = cleo->ReadParam(handle)->f;
    float radiusZ = cleo->ReadParam(handle)->f;

    float rx = cleo->ReadParam(handle)->f; // rotX
    float ry = cleo->ReadParam(handle)->f; // rotY
    float rz = cleo->ReadParam(handle)->f; // rotZ

    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;
    float cz = cleo->ReadParam(handle)->f;

    if (angleMode == 0) {
        ax = DegToRadF(ax);
        ay = DegToRadF(ay);
        rx = DegToRadF(rx);
        ry = DegToRadF(ry);
        rz = DegToRadF(rz);
    }

    float sax = sinf(ax);
    float cax = cosf(ax);
    float say = sinf(ay);
    float cay = cosf(ay);

    float x = sax * cay * radiusX;
    float y = sax * say * radiusY;
    float z =      cax * radiusZ;

    // 👉 Rotación 3D opcional
    if (rx != 0.0f || ry != 0.0f || rz != 0.0f)
    {
        // Rot X
        if (rx != 0.0f) {
            float s = sinf(rx), c = cosf(rx);
            float ny = y * c - z * s;
            float nz = y * s + z * c;
            y = ny; z = nz;
        }

        // Rot Y
        if (ry != 0.0f) {
            float s = sinf(ry), c = cosf(ry);
            float nx = x * c + z * s;
            float nz = -x * s + z * c;
            x = nx; z = nz;
        }

        // Rot Z
        if (rz != 0.0f) {
            float s = sinf(rz), c = cosf(rz);
            float nx = x * c - y * s;
            float ny = x * s + y * c;
            x = nx; y = ny;
        }
    }

    cleo->GetPointerToScriptVar(handle)->f = x + cx;
    cleo->GetPointerToScriptVar(handle)->f = y + cy;
    cleo->GetPointerToScriptVar(handle)->f = z + cz;
}


// Normalize helper
static inline void Normalize(float& x, float& y, float& z) {
    // skip entirely if it's already unit-ish
    const float len2 = x*x + y*y + z*z;
    if (len2 > 1.000002f || len2 < 0.999998f) {
        float len = sqrtf(len2);
        if (len > 0.000001f) {
            x /= len; y /= len; z /= len;
        }
    }
}


// Cross product
static inline void Cross(float ax, float ay, float az, float bx, float by, float bz,
                         float& rx, float& ry, float& rz)
{
    // si ambos vectores son cero → resultado es cero
    if ((ax == 0.0f && ay == 0.0f && az == 0.0f) ||
        (bx == 0.0f && by == 0.0f && bz == 0.0f))
    {
        rx = ry = rz = 0.0f;
        return;
    }

    rx = ay*bz - az*by;
    ry = az*bx - ax*bz;
    rz = ax*by - ay*bx;
}


// 701B=15,%13d% %14d% %15d% = orbit_cylinder %1b:angle/radian% angle %2d% level %3d% height %4d% radius %5d% %6d% rotation %7d% %8d% %9d% coords %10d% %11d% %12d%
// params:
// 1: angleMode (0° / 1 rad)
// 2: angle
// 3: level (height along cylinder axis)
// 4: maxHeight
// 5: radiusStart
// 6: radiusEnd
// 7,8,9: rotX rotY rotZ (rotation of cylinder)
// 10,11,12: centerX centerY centerZ
CLEO_Fn(ORBIT_CYLINDER)
{
    int angleMode   = cleo->ReadParam(handle)->i;
    float angle     = cleo->ReadParam(handle)->f;
    float level     = cleo->ReadParam(handle)->f;
    float maxH      = cleo->ReadParam(handle)->f;
    float r1        = cleo->ReadParam(handle)->f;
    float r2        = cleo->ReadParam(handle)->f;

    float dx = cleo->ReadParam(handle)->f;
    float dy = cleo->ReadParam(handle)->f;
    float dz = cleo->ReadParam(handle)->f;

    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;
    float cz = cleo->ReadParam(handle)->f;

    if (angleMode == 0) {
        angle = DegToRadF(angle);

        if (dx != 0.0f) dx = DegToRadF(dx);
        if (dy != 0.0f) dy = DegToRadF(dy);
        if (dz != 0.0f) dz = DegToRadF(dz);
    }

    // Normalize direction ONLY if needed
    if (!(dx == 0.0f && dy == 0.0f && dz == 0.0f))
        Normalize(dx, dy, dz);

    float t = (maxH != 0.0f) ? (level / maxH) : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float radius = r1 + (r2 - r1) * t;

    // if direction is 0,0,0 → use default UP vector
    if (dx == 0.0f && dy == 0.0f && dz == 0.0f)
        dz = 1.0f;

    float ux, uy, uz;
    float vx, vy, vz;

    float auxX = (fabs(dx) > 0.9f) ? 0.0f : 1.0f;
    float auxY = 0.0f;
    float auxZ = (fabs(dx) > 0.9f) ? 1.0f : 0.0f;

    Cross(dx, dy, dz, auxX, auxY, auxZ, ux, uy, uz);
    Normalize(ux, uy, uz);

    Cross(dx, dy, dz, ux, uy, uz, vx, vy, vz);
    Normalize(vx, vy, vz);

    float px = cx + dx*level + (ux*cosf(angle) + vx*sinf(angle)) * radius;
    float py = cy + dy*level + (uy*cosf(angle) + vy*sinf(angle)) * radius;
    float pz = cz + dz*level + (uz*cosf(angle) + vz*sinf(angle)) * radius;

    cleo->GetPointerToScriptVar(handle)->f = px;
    cleo->GetPointerToScriptVar(handle)->f = py;
    cleo->GetPointerToScriptVar(handle)->f = pz;
}

// 701D=14,%12d% %13d% %14d% = orbit_polygon %1b:angle/radian% angle %2d% sides %3d% radius %4d% smooth %5d% rotation %6d% %7d% %8d% coords %9d% %10d% %11d%
// params:
// 1: angleMode (0 = degrees, 1 = radians)
// 2: angle
// 3: sides
// 4: radius
// 5,6,7: rotX rotY rotZ   (rotation in 3D space)
// 8,9,10: centerX centerY centerZ
CLEO_Fn(ORBIT_POLYGON)
{
    int angleMode = cleo->ReadParam(handle)->i;
    float angle   = cleo->ReadParam(handle)->f;
    int sides     = cleo->ReadParam(handle)->i;
    float radius  = cleo->ReadParam(handle)->f;

    float smooth  = cleo->ReadParam(handle)->f;

    float rotX    = cleo->ReadParam(handle)->f;
    float rotY    = cleo->ReadParam(handle)->f;
    float rotZ    = cleo->ReadParam(handle)->f;

    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;
    float cz = cleo->ReadParam(handle)->f;

    if (sides < 3) sides = 3;

    // Convert initial angle
    if (angleMode == 0)
        angle = DegToRadF(angle);

    const float TWO_PI = 6.28318530717958647692f;

    angle = fmodf(angle, TWO_PI);
    if (angle < 0) angle += TWO_PI;

    float step = TWO_PI / (float)sides;

    // Convert rotations only if needed
    bool useRot = (rotX != 0.0f || rotY != 0.0f || rotZ != 0.0f);

    if (angleMode == 0 && useRot)
    {
        rotX = DegToRadF(rotX);
        rotY = DegToRadF(rotY);
        rotZ = DegToRadF(rotZ);
    }

    // ---------------------------
    // Cálculo del polígono base
    // ---------------------------

    float frac = angle / TWO_PI;
    float a = frac * TWO_PI;

    int segIndex = (int)(a / step);
    float localT = (a - segIndex * step) / step;

    segIndex %= sides;

    int i0 = segIndex;
    int i1 = (i0 + 1) % sides;

    float ang0 = step * i0;
    float ang1 = step * i1;

    float x0 = radius * cosf(ang0);
    float y0 = radius * sinf(ang0);

    float x1 = radius * cosf(ang1);
    float y1 = radius * sinf(ang1);

    // ---------------------------
    // Interpolación suave
    // ---------------------------
    float rawX = LerpF(x0, x1, localT);
    float rawY = LerpF(y0, y1, localT);

    float circX = radius * cosf(angle);
    float circY = radius * sinf(angle);

    // Mezcla entre polígono duro y círculo
    float x = LerpF(rawX, circX, smooth);
    float y = LerpF(rawY, circY, smooth);
    float z = 0.0f;

    // ---------------------------
    // Rotaciones (solo si se necesitan)
    // ---------------------------
    if (useRot)
    {
        if (rotX != 0.0f) {
            float s = sinf(rotX), c = cosf(rotX);
            float ny = y * c - z * s;
            float nz = y * s + z * c;
            y = ny; z = nz;
        }

        if (rotY != 0.0f) {
            float s = sinf(rotY), c = cosf(rotY);
            float nx = x * c + z * s;
            float nz = -x * s + z * c;
            x = nx; z = nz;
        }

        if (rotZ != 0.0f) {
            float s = sinf(rotZ), c = cosf(rotZ);
            float nx = x * c - y * s;
            float ny = x * s + y * c;
            x = nx; y = ny;
        }
    }

    // ---------------------------
    // Agregar centro
    // ---------------------------
    x += cx;
    y += cy;
    z += cz;

    cleo->GetPointerToScriptVar(handle)->f = x;
    cleo->GetPointerToScriptVar(handle)->f = y;
    cleo->GetPointerToScriptVar(handle)->f = z;
}


// 701E=16,%14d% %15d% %16d% = orbit_cube %1b:angle/radian% angles %2d% %3d% size %4d% %5d% %6d% smooth %7d% rotation %8d% %9d% %10d% coords %11d% %12d% %13d%
// params:
// 1: angleMode (0° / 1 rad)
// 2,3: angles X Y
// 4,5,6: sizeX sizeY sizeZ
// 7: smooth (0.0 = cube duro / 1.0 = total ovoide)
// 8,9,10: rotX rotY rotZ (rotation applied to final point)
// 11,12,13: centerX centerY centerZ
CLEO_Fn(ORBIT_CUBE)
{
    int angleMode = cleo->ReadParam(handle)->i;

    float angX = cleo->ReadParam(handle)->f;
    float angY = cleo->ReadParam(handle)->f;

    float sizeX = cleo->ReadParam(handle)->f;
    float sizeY = cleo->ReadParam(handle)->f;
    float sizeZ = cleo->ReadParam(handle)->f;

    float smooth = cleo->ReadParam(handle)->f;

    float rotX = cleo->ReadParam(handle)->f;
    float rotY = cleo->ReadParam(handle)->f;
    float rotZ = cleo->ReadParam(handle)->f;

    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;
    float cz = cleo->ReadParam(handle)->f;

    // ---------------------------
    // Convertir ángulos si hace falta
    // ---------------------------
    if (angleMode == 0)
    {
        angX = DegToRadF(angX);
        angY = DegToRadF(angY);

        if (rotX != 0.0f) rotX = DegToRadF(rotX);
        if (rotY != 0.0f) rotY = DegToRadF(rotY);
        if (rotZ != 0.0f) rotZ = DegToRadF(rotZ);
    }

    // ---------------------------
    // Calcular posición cúbica sin suavizado
    // ---------------------------

    float sx = sinf(angX);
    float cxA = cosf(angX);
    float sy = sinf(angY);
    float cyA = cosf(angY);

    // Para cubo: tomamos signos "duros"
    float baseX = (sx >= 0 ? sizeX : -sizeX);
    float baseY = (sy >= 0 ? sizeY : -sizeY);
    float baseZ = (cxA >= 0 ? sizeZ : -sizeZ);

    // ---------------------------
    // Crear ovoide interpolado (smooth)
    // ---------------------------
    float ox = sx * sizeX;
    float oy = sy * sizeY;
    float oz = cxA * sizeZ;

    float px = baseX * (1.0f - smooth) + ox * smooth;
    float py = baseY * (1.0f - smooth) + oy * smooth;
    float pz = baseZ * (1.0f - smooth) + oz * smooth;

    // ---------------------------
    // Rotación extra del cubo / ovoide (si no es 0 evita cálculo)
    // ---------------------------
    // Rotación en X
    if (rotX != 0.0f)
    {
        float s = sinf(rotX), c = cosf(rotX);
        float ny = py * c - pz * s;
        float nz = py * s + pz * c;
        py = ny; pz = nz;
    }

    // Rotación en Y
    if (rotY != 0.0f)
    {
        float s = sinf(rotY), c = cosf(rotY);
        float nx = px * c + pz * s;
        float nz = -px * s + pz * c;
        px = nx; pz = nz;
    }

    // Rotación en Z
    if (rotZ != 0.0f)
    {
        float s = sinf(rotZ), c = cosf(rotZ);
        float nx = px * c - py * s;
        float ny = px * s + py * c;
        px = nx; py = ny;
    }

    // ---------------------------
    // Resultado final + centro
    // ---------------------------
    cleo->GetPointerToScriptVar(handle)->f = px + cx;
    cleo->GetPointerToScriptVar(handle)->f = py + cy;
    cleo->GetPointerToScriptVar(handle)->f = pz + cz;
}

// 701F=11,%9d% %10d% = orbit_square %1b:angle/radian% angle %2d% size %3d% %4d% smooth %5d% rotZ %6d% coords %7d% %8d%
// params:
// 1: angleMode (0° / 1 rad)
// 2: angle
// 3,4: sizeX sizeY
// 5: smooth (0.0 = square duro / 1.0 = oval)
// 6: rotZ
// 7,8: centerX centerY
CLEO_Fn(ORBIT_SQUARE)
{
    int angleMode = cleo->ReadParam(handle)->i;

    float ang = cleo->ReadParam(handle)->f;

    float sizeX = cleo->ReadParam(handle)->f;
    float sizeY = cleo->ReadParam(handle)->f;

    float smooth = cleo->ReadParam(handle)->f;
    float rotZ = cleo->ReadParam(handle)->f;

    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;

    // Convertir ángulo si hace falta
    if (angleMode == 0)
        ang = ang * 0.017453292519943295f;

    float s = sinf(ang);
    float c = cosf(ang);

    // Coordenadas cuadradas puras (hard edges)
    float baseX = (s >= 0 ? sizeX : -sizeX);
    float baseY = (c >= 0 ? sizeY : -sizeY);

    // Coordenadas suaves (círculo/oval)
    float ox = s * sizeX;
    float oy = c * sizeY;

    // Interpolación cuadrado ↔ círculo
    float px = baseX * (1.0f - smooth) + ox * smooth;
    float py = baseY * (1.0f - smooth) + oy * smooth;

    // Rotación Z opcional
    if (rotZ != 0.0f)
    {
        float sz = sinf(rotZ);
        float cz = cosf(rotZ);
        float nx = px * cz - py * sz;
        float ny = px * sz + py * cz;
        px = nx; py = ny;
    }

    // Resultado final
    cleo->GetPointerToScriptVar(handle)->f = px + cx;
    cleo->GetPointerToScriptVar(handle)->f = py + cy;
}


///////////////////////////////////////////////////
/////////////////// ANIMATION /////////////////////
///////////////////////////////////////////////////

// Helpers: reinterpret float bits
inline bool FloatIsNegative(float v) {
    uint32_t bits = *(uint32_t*)&v;
    return (bits >> 31) != 0;      // sign bit
}

inline float FloatAbsRaw(float v) {
    uint32_t bits = *(uint32_t*)&v;
    bits &= 0x7FFFFFFF;            // clear sign bit
    return *(float*)&bits;
}

inline float FloatSetSign(float v, bool negative) {
    uint32_t bits = *(uint32_t*)&v;
    if (negative) bits |= 0x80000000;
    else bits &= 0x7FFFFFFF;
    return *(float*)&bits;
}

// 7021=12,%10d% %11d% %12d% progress %9d% = move_lerp %1d% %2d% %3d% to %4d% %5d% %6d% deltatime %7d% speed %8d%
CLEO_Fn(MOVE_LERP)
{
    // PARAMETERS
    float x0 = cleo->ReadParam(handle)->f;
    float y0 = cleo->ReadParam(handle)->f;
    float z0 = cleo->ReadParam(handle)->f;

    float x1 = cleo->ReadParam(handle)->f;
    float y1 = cleo->ReadParam(handle)->f;
    float z1 = cleo->ReadParam(handle)->f;

    float dt    = cleo->ReadParam(handle)->f;   // delta time in seconds
    float speed = cleo->ReadParam(handle)->f;   // meters/sec

    // RAW pointer to progress
    float* tPtr = &cleo->GetPointerToScriptVar(handle)->f;

    float rawT   = *tPtr;
    bool reverse = FloatIsNegative(rawT);
    float t      = FloatAbsRaw(rawT);

    // distance in meters
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dz = z1 - z0;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);

    // no movement
    if (dist <= 0.000001f) {
        float finalT = reverse ? -1.0f : 1.0f;
        *tPtr = finalT;

        cleo->GetPointerToScriptVar(handle)->f = x1; // x
        cleo->GetPointerToScriptVar(handle)->f = y1; // y
        cleo->GetPointerToScriptVar(handle)->f = z1; // z
        return;
    }

    // PROGRESS DELTA (correct physical interpretation)
    float dtProgress = (speed * dt) / dist;

    // Apply reverse logic
    t += reverse ? -dtProgress : dtProgress;

    // Clamp
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;

    // store signed t
    *tPtr = reverse ? -t : t;

    // actual interpolation param
    float tActual = reverse ? (1.0f - t) : t;

    // final coords
    float x = x0 + dx * tActual;
    float y = y0 + dy * tActual;
    float z = z0 + dz * tActual;

    cleo->GetPointerToScriptVar(handle)->f = x; // x
    cleo->GetPointerToScriptVar(handle)->f = y; // y
    cleo->GetPointerToScriptVar(handle)->f = z; // z
}

// 7022=1,  lerp_is_finished %1d%
CLEO_Fn(LERP_IS_FINISHED)
{
    float progress = cleo->ReadParam(handle)->f;
    float absP = FloatAbsRaw(progress);
    UpdateCompareFlag(handle, absP >= 1.0f);
}


// 7024=6,%6d% progress %5d% = value_lerp %1d% to %2d% deltatime %3d% speed %4d%
CLEO_Fn(VALUE_LERP)
{
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;

    float dt    = cleo->ReadParam(handle)->f;   // delta time in seconds
    float speed = cleo->ReadParam(handle)->f;   // meters/sec

    float* tPtr = &cleo->GetPointerToScriptVar(handle)->f; // progress storage
    float rawT = *tPtr;
    bool reverse = FloatIsNegative(rawT);
    float t = FloatAbsRaw(rawT);

    float diff = b - a;
    float dist = fabsf(diff);

    if (dist <= 0.000001f) {
        *tPtr = reverse ? -1.0f : 1.0f;
        cleo->GetPointerToScriptVar(handle)->f = b;
        return;
    }

    float dtProgress = (speed * dt) / dist;
    t += reverse ? -dtProgress : dtProgress;

    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;

    *tPtr = reverse ? -t : t;

    float tActual = reverse ? (1.0f - t) : t;
    float result = a + diff * tActual;

    cleo->GetPointerToScriptVar(handle)->f = result;
}

// 7025=1,lerp_maintain_loop %1d%
CLEO_Fn(LERP_MAINTAIN_LOOP)
{
    float* progress = &cleo->GetPointerToScriptVar(handle)->f;
    float raw = *progress;
    bool negative = FloatIsNegative(raw);
    float absP = FloatAbsRaw(raw);

    // if >=1.0 then wrap to 0.0 and preserve direction
    if (absP >= 1.0f) {
        float newP = 0.0f;
        *progress = FloatSetSign(newP, negative);
    }
}



// ==================== ESTRUCTURAS Y CONSTANTES ====================

struct Vec3 {
    float x, y, z;
};

struct Vec2 {
    float x, y;
};

// ==================== FUNCIONES AUXILIARES ====================

// Interpolación lineal simple entre dos valores
inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Interpolación lineal para Vec3
inline Vec3 lerp3D(const Vec3& a, const Vec3& b, float t) {
    Vec3 result;
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    return result;
}

// Aplica una curva al factor de interpolación (igual que tu código original)
float applyCurve(float t, int mode, float p1, float p2, float p3, float p4) {
    float y = 0.0f;
    
    enum CURVE_MODE {
        CURVE_LINEAR = 0,
        CURVE_SMOOTHSTEP = 1,
        CURVE_EASE_IN = 2,
        CURVE_EASE_OUT = 3,
        CURVE_EASE_IN_OUT = 4,
        CURVE_POWER_GENERAL = 5,
        CURVE_CUBIC_BEZIER = 6,
        CURVE_SPRING = 7,
        CURVE_ELASTIC = 8,
        CURVE_BOUNCE = 9,
        CURVE_STEPPED = 10,
        CURVE_OVERSHOOT_SPRING = 11
    };

    switch (mode) {
        // 0 — LINEAR
        default:
        case CURVE_MODE::CURVE_LINEAR:
            y = t;
            break;

        // 1 — SMOOTHSTEP
        case CURVE_MODE::CURVE_SMOOTHSTEP:
            y = t*t*(3 - 2*t);
            break;

        // 2 — EASE IN (power curve)
        case CURVE_MODE::CURVE_EASE_IN:
            y = powf(t, p1 <= 0 ? 2.0f : p1);
            break;

        // 3 — EASE OUT (power curve)
        case CURVE_MODE::CURVE_EASE_OUT:
            y = 1.0f - powf(1.0f - t, p1 <= 0 ? 2.0f : p1);
            break;

        // 4 — EASE IN OUT (power)
        case CURVE_MODE::CURVE_EASE_IN_OUT: {
            float exp = p1 <= 0 ? 2.0f : p1;
            if (t < 0.5f) y = 0.5f * powf(t * 2.0f, exp);
            else y = 1.0f - 0.5f * powf((1.0f - t) * 2.0f, exp);
            break;
        }

        // 5 — POWER CURVE GENERAL (powf(t, p1+p2))
        case CURVE_MODE::CURVE_POWER_GENERAL:
            y = powf(t, p1 + p2);
            break;

        // 6 — CUBIC BEZIER (p1,p2,p3,p4)
        case CURVE_MODE::CURVE_CUBIC_BEZIER: {
            float u = 1 - t;
            y = u*u*u * p1 +
                3*u*u*t * p2 +
                3*u*t*t * p3 +
                t*t*t * p4;
            break;
        }

        // 7 — SPRING (p1 stiffness, p2 damping)
        case CURVE_MODE::CURVE_SPRING: {
            float w = p1 <= 0 ? 8.0f : p1;  // stiffness
            float d = p2 <= 0 ? 0.3f : p2;  // damping
            y = 1 - expf(-t * w) * cosf(t * w * (1.0f - d));
            break;
        }

        // 8 — ELASTIC (p1 amplitude, p2 frequency)
        case CURVE_MODE::CURVE_ELASTIC: {
            float amp = p1 == 0 ? 1.0f : p1;
            float freq = p2 == 0 ? 10.0f : p2;
            y = powf(2, -10*t) * sinf((t - p3) * (float)M_PI * freq) * amp + 1;
            break;
        }

        // 9 — BOUNCE (p1 factor)
        case CURVE_MODE::CURVE_BOUNCE: {
            float k = p1 <= 0 ? 2.0f : p1;
            y = fabsf(sinf(t * (float)M_PI * k)) * t;
            break;
        }

        // 10 — STEPPED (p1 = cantidad de pasos)
        case CURVE_MODE::CURVE_STEPPED: {
            int steps = (int)(p1 < 1 ? 1 : p1);
            y = floorf(t * steps) / steps;
            break;
        }

        // 11 — OVERSHOOT SPRING (tipo cartoon)
        case CURVE_MODE::CURVE_OVERSHOOT_SPRING: {
            float stiff = p1 == 0 ? 12.0f : p1;
            float damp = p2 == 0 ? 0.2f : p2;
            y = 1 + expf(-t * stiff) * sinf(t * stiff * (1 - damp));
            break;
        }
    }
    
    return y;
}

// Calcula la distancia 3D entre dos puntos
float distance3D(const Vec3& a, const Vec3& b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dz = b.z - a.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

// 7026=12,%12d% progress %11d% = value_lerp_curved  %1d% to %2d% dt %3d% speed %4d% mode %5d% params %6d% %7d% %8d% %9d% overshoot %10d%
CLEO_Fn(VALUE_LERP_CURVED) {
    float a = cleo->ReadParam(handle)->f;
    float b = cleo->ReadParam(handle)->f;
    float dt = cleo->ReadParam(handle)->f;        // seconds
    float speed = cleo->ReadParam(handle)->f;     // units/sec
    int mode = cleo->ReadParam(handle)->i;
    float p1 = cleo->ReadParam(handle)->f;
    float p2 = cleo->ReadParam(handle)->f;
    float p3 = cleo->ReadParam(handle)->f;
    float p4 = cleo->ReadParam(handle)->f;
    bool overshoot = cleo->ReadParam(handle)->i != 0;

    float* tPtr = &cleo->GetPointerToScriptVar(handle)->f;
    float rawT = *tPtr;
    bool reverse = FloatIsNegative(rawT);
    float t = FloatAbsRaw(rawT);

    float diff = b - a;
    float dist = fabsf(diff);
    if (dist < 0.000001f) {
        *tPtr = reverse ? -1.0f : 1.0f;
        cleo->GetPointerToScriptVar(handle)->f = b;
        return;
    }

    float tAdd = (speed * dt) / dist;
    t += reverse ? -tAdd : tAdd;

    if (!overshoot) {
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;
    }
    *tPtr = reverse ? -t : t;

    float tv = reverse ? (1.0f - t) : t;
    float y = applyCurve(tv, mode, p1, p2, p3, p4);
    float result = a + diff * y;

    cleo->GetPointerToScriptVar(handle)->f = result;
}

// 7027=18,%16d% %17d% %18d% progress %15d% = move_lerp_curved %1d% %2d% %3d% to %4d% %5d% %6d% dt %7d% speed %8d% mode %9d% params %10d% %11d% %12d% %13d% overshoot %14d%
// params:
// 1,2,3: position A (x,y,z)
// 4,5,6: position B (x,y,z)
// 7: delta time (seconds)
// 8: speed (meters/sec)
// 9: curve mode
// 10,11,12,13: curve params (p1,p2,p3,p4)
// 14: overshoot (bool)
// 15: progress storage (float)
// 16,17,18: result position (x,y,z)
CLEO_Fn(MOVE_LERP_CURVED) {
    Vec3 a;
    a.x = cleo->ReadParam(handle)->f;
    a.y = cleo->ReadParam(handle)->f;
    a.z = cleo->ReadParam(handle)->f;
    Vec3 b;
    b.x = cleo->ReadParam(handle)->f;
    b.y = cleo->ReadParam(handle)->f;
    b.z = cleo->ReadParam(handle)->f;

    float dt = cleo->ReadParam(handle)->f;      // seconds
    float speed = cleo->ReadParam(handle)->f;   // meters/sec
    int mode = cleo->ReadParam(handle)->i;
    float p1 = cleo->ReadParam(handle)->f;
    float p2 = cleo->ReadParam(handle)->f;
    float p3 = cleo->ReadParam(handle)->f;
    float p4 = cleo->ReadParam(handle)->f;
    bool overshoot = cleo->ReadParam(handle)->i != 0;

    float* tPtr = &cleo->GetPointerToScriptVar(handle)->f; // progress
    float rawT = *tPtr;
    bool reverse = FloatIsNegative(rawT);
    float t = FloatAbsRaw(rawT);

    float dist = distance3D(a, b);
    if (dist < 0.000001f) {
        *tPtr = reverse ? -1.0f : 1.0f;
        cleo->GetPointerToScriptVar(handle)->f = b.x;
        cleo->GetPointerToScriptVar(handle)->f = b.y;
        cleo->GetPointerToScriptVar(handle)->f = b.z;
        return;
    }

    float tAdd = (speed * dt) / dist;
    t += reverse ? -tAdd : tAdd;

    if (!overshoot) {
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;
    }
    *tPtr = reverse ? -t : t;

    float tv = reverse ? (1.0f - t) : t;
    float curveFactor = applyCurve(tv, mode, p1, p2, p3, p4);

    Vec3 res = lerp3D(a, b, curveFactor);

    cleo->GetPointerToScriptVar(handle)->f = res.x;
    cleo->GetPointerToScriptVar(handle)->f = res.y;
    cleo->GetPointerToScriptVar(handle)->f = res.z;
}

// 7028=1,toggle_lerp_reverse %1d%
CLEO_Fn(TOGGLE_LERP_REVERSE)
{
    float* t = &cleo->GetPointerToScriptVar(handle)->f;
    uint32_t bits = *(uint32_t*)t;
    bits ^= 0x80000000;           // flip sign bit
    *t = *(float*)&bits;
}

// Shortest signed angle delta (from -> to) in degrees, range [-180, 180)
static inline float AngleDeltaSigned(float from, float to) {
    float diff = fmodf(to - from + 180.0f, 360.0f);
    if (diff < 0.0f) diff += 360.0f;
    diff -= 180.0f;
    return diff;
}

// 7020=6,%6d% progress %5d% = rotate_lerp %1d% %2d% dt %3d% speed %4d%
CLEO_Fn(ROTATE_LERP)
{
    float a0    = cleo->ReadParam(handle)->f; // start angle (deg)
    float a1    = cleo->ReadParam(handle)->f; // end angle   (deg)
    float dt    = cleo->ReadParam(handle)->f; // seconds
    float speed = cleo->ReadParam(handle)->f; // degrees / sec

    float* tPtr = &cleo->GetPointerToScriptVar(handle)->f;
    float rawT  = *tPtr;
    bool reverse = FloatIsNegative(rawT);
    float t = FloatAbsRaw(rawT);

    float diff = AngleDeltaSigned(a0, a1);
    float dist = fabsf(diff);

    if (dist < 1e-6f) {
        *tPtr = reverse ? -1.0f : 1.0f;
        cleo->GetPointerToScriptVar(handle)->f = a1;
        return;
    }

    float tAdd = (speed * dt) / dist;
    t += reverse ? -tAdd : tAdd;

    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;

    *tPtr = reverse ? -t : t;

    float tActual = reverse ? (1.0f - t) : t;

    float outAngle = a0 + diff * tActual;

    cleo->GetPointerToScriptVar(handle)->f = outAngle;
}

// 7028=12,%12d% progress %11d% = rotate_lerp_curved %1d% %2d% dt %3d% speed %4d% mode %5d% params %6d% %7d% %8d% %9d% overshoot %10d%
CLEO_Fn(ROTATE_LERP_CURVED)
{
    float a0    = cleo->ReadParam(handle)->f;
    float a1    = cleo->ReadParam(handle)->f;
    float dt    = cleo->ReadParam(handle)->f;    // seconds
    float speed = cleo->ReadParam(handle)->f;    // degrees/sec

    int mode    = cleo->ReadParam(handle)->i;
    float p1    = cleo->ReadParam(handle)->f;
    float p2    = cleo->ReadParam(handle)->f;
    float p3    = cleo->ReadParam(handle)->f;
    float p4    = cleo->ReadParam(handle)->f;
    bool overshoot = cleo->ReadParam(handle)->i != 0;

    float* tPtr = &cleo->GetPointerToScriptVar(handle)->f; // progress storage
    float rawT  = *tPtr;
    bool reverse = FloatIsNegative(rawT);
    float t = FloatAbsRaw(rawT);

    float diff = AngleDeltaSigned(a0, a1);
    float dist = fabsf(diff);

    if (dist < 1e-6f) {
        *tPtr = reverse ? -1.0f : 1.0f;
        cleo->GetPointerToScriptVar(handle)->f = a1;
        return;
    }

    float tAdd = (speed * dt) / dist;
    t += reverse ? -tAdd : tAdd;

    if (!overshoot) {
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;
    }

    *tPtr = reverse ? -t : t;

    float tv = reverse ? (1.0f - t) : t;
    float curveFactor = applyCurve(tv, mode, p1, p2, p3, p4);

    float outAngle = a0 + diff * curveFactor;

    cleo->GetPointerToScriptVar(handle)->f = outAngle;
}

/*
70XX=??, x %out% y %out% z %out% progress %p%
= quadratic_lerp_curved

P0_1(xyz) P1_1(xyz) P2_1(xyz)
P0_2(xyz) P1_2(xyz) P2_2(xyz)
P0_3(xyz) P1_3(xyz) P2_3(xyz)

dt %dt%
speed %speed%
mode %mode%
params %pA% %pB% %pC% %pD%
overshoot %over%
 */

//---------------------------------------------------


// 7029=21,%19d% %20d% %21d% progress %18d% = quadratic_lerp_curved %1d% %2d% %3d% per %4d% %5d% %6d% to %7d% %8d% %9d% dt %10d% speed %11d% mode %12d% params %13d% %14d% %15d% %16d% overshoot %17d%
// params:
// 1,2,3: position A (x,y,z)
// 4,5,6: position B (x,y,z)
// 7,8,9: position C (x,y,z)
// 10: delta time (seconds)
// 11: speed (meters/sec)
// 12: curve mode
// 13,14,15,16: curve params (p1,p2,p3,p4)
// 17: overshoot (bool)
// 18: progress storage (float)
// 19,20,21: result position (x,y,z)
CLEO_Fn(QUADRATIC_LERP_CURVED)
{
    Vec3 P0, P1, P2;

    P0.x = cleo->ReadParam(handle)->f;
    P0.y = cleo->ReadParam(handle)->f;
    P0.z = cleo->ReadParam(handle)->f;

    P1.x = cleo->ReadParam(handle)->f;
    P1.y = cleo->ReadParam(handle)->f;
    P1.z = cleo->ReadParam(handle)->f;

    P2.x = cleo->ReadParam(handle)->f;
    P2.y = cleo->ReadParam(handle)->f;
    P2.z = cleo->ReadParam(handle)->f;

    float dt    = cleo->ReadParam(handle)->f;
    float speed = cleo->ReadParam(handle)->f;
    int mode    = cleo->ReadParam(handle)->i;

    float pA = cleo->ReadParam(handle)->f;
    float pB = cleo->ReadParam(handle)->f;
    float pC = cleo->ReadParam(handle)->f;
    float pD = cleo->ReadParam(handle)->f;

    bool overshoot = cleo->ReadParam(handle)->i != 0;

    float* pProg = &cleo->GetPointerToScriptVar(handle)->f;
    float rawT = *pProg;
    bool reverse = FloatIsNegative(rawT);
    float t = FloatAbsRaw(rawT);

    float approxLen = distance3D(P0, P1) + distance3D(P1, P2);
    if (approxLen < 1e-6f) approxLen = 1e-6f;

    float tAdd = (speed * dt) / approxLen;
    t += reverse ? -tAdd : tAdd;

    if (!overshoot) {
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    }

    *pProg = reverse ? -t : t;

    float tv = reverse ? (1.0f - t) : t;
    float curvedT = applyCurve(tv, mode, pA, pB, pC, pD);

    float u  = 1.0f - curvedT;
    float u2 = u*u;
    float t2 = curvedT*curvedT;

    Vec3 out;
    out.x = u2*P0.x + 2*u*curvedT*P1.x + t2*P2.x;
    out.y = u2*P0.y + 2*u*curvedT*P1.y + t2*P2.y;
    out.z = u2*P0.z + 2*u*curvedT*P1.z + t2*P2.z;

    cleo->GetPointerToScriptVar(handle)->f = out.x;
    cleo->GetPointerToScriptVar(handle)->f = out.y;
    cleo->GetPointerToScriptVar(handle)->f = out.z;
}

// 702A=10,%8d% %9d% %10d% = blend_VEC3 %1d% %2d% %3d% to %4d% %5d% %6d% blend %7d%
CLEO_Fn(BLEND_VEC3)
{
    Vec3 A, B;

    A.x = cleo->ReadParam(handle)->f;
    A.y = cleo->ReadParam(handle)->f;
    A.z = cleo->ReadParam(handle)->f;

    B.x = cleo->ReadParam(handle)->f;
    B.y = cleo->ReadParam(handle)->f;
    B.z = cleo->ReadParam(handle)->f;

    float blend = cleo->ReadParam(handle)->f;

    // Clamp opcional por seguridad
    if (blend < 0.0f) blend = 0.0f;
    if (blend > 1.0f) blend = 1.0f;

    Vec3 out;
    out.x = A.x * (1.0f - blend) + B.x * blend;
    out.y = A.y * (1.0f - blend) + B.y * blend;
    out.z = A.z * (1.0f - blend) + B.z * blend;

    cleo->GetPointerToScriptVar(handle)->f = out.x;
    cleo->GetPointerToScriptVar(handle)->f = out.y;
    cleo->GetPointerToScriptVar(handle)->f = out.z;
}

// 702B=7,%6d% %7d% = blend_vec2 %1d% %2d% to %3d% %4d% blend %5d%
CLEO_Fn(BLEND_VEC2)
{
    Vec2 A, B;

    A.x = cleo->ReadParam(handle)->f;
    A.y = cleo->ReadParam(handle)->f;

    B.x = cleo->ReadParam(handle)->f;
    B.y = cleo->ReadParam(handle)->f;

    float blend = cleo->ReadParam(handle)->f;

    // Clamp opcional por seguridad
    if (blend < 0.0f) blend = 0.0f;
    if (blend > 1.0f) blend = 1.0f;

    Vec2 out;
    out.x = A.x * (1.0f - blend) + B.x * blend;
    out.y = A.y * (1.0f - blend) + B.y * blend;

    cleo->GetPointerToScriptVar(handle)->f = out.x;
    cleo->GetPointerToScriptVar(handle)->f = out.y;
}

// 702C=1,  is_lerp_overshooting %1d%
CLEO_Fn(IS_LERP_OVERSHOOTING)
{
    float progress = cleo->ReadParam(handle)->f;
    float absP = FloatAbsRaw(progress);
    UpdateCompareFlag(handle, absP > 1.0f);
}


///////////////////////////////////////////////////
///////////////// ARRAY BUFFER ////////////////////
///////////////////////////////////////////////////

static const int EMPTY_SLOT = -2147483647;
static const int INVALID_SLOT = -2147483646;

// 702D=4,write_array_safe %1d% length %2d% index %3d% value %4d%
CLEO_Fn(WRITE_ARRAY_SAFE)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);

    uint32_t* base = reinterpret_cast<uint32_t*>(raw);
    int length = cleo->ReadParam(handle)->i;
    int index  = cleo->ReadParam(handle)->i;
    uint32_t value = (uint32_t)cleo->ReadParam(handle)->i;

    if (length <= 0) return;
    if (index < 0 || index >= length) return;

    base[index] = value;
}


// 702E=4,%4d% = read_array_safe %1d% length %2d% index %3d%
CLEO_Fn(READ_ARRAY_SAFE)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);

    int result = INVALID_SLOT;
    uint32_t* base = reinterpret_cast<uint32_t*>(raw);
    int length = cleo->ReadParam(handle)->i;
    int index  = cleo->ReadParam(handle)->i;

    if (length > 0 && index >= 0 && index < length)
    {
        if (base[index] != EMPTY_SLOT)
            result = base[index];
    }
    
    UpdateCompareFlag(handle, result != INVALID_SLOT);

    cleo->GetPointerToScriptVar(handle)->i = result;
}


// 702F=4,count_array_slots %1d% length %2d% empty %3d% used %4d%
CLEO_Fn(COUNT_ARRAY_SLOTS)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);

    int empty = INVALID_SLOT, used = INVALID_SLOT;

    uint32_t* base = reinterpret_cast<uint32_t*>(raw);
    int length = cleo->ReadParam(handle)->i;

    if (length > 0)
    {
        empty = used = 0;

        for (int i = 0; i < length; i++)
        {
            if (base[i] == EMPTY_SLOT) empty++;
            else used++;
        }
    }

    cleo->GetPointerToScriptVar(handle)->i = empty;
    cleo->GetPointerToScriptVar(handle)->i = used;
}


// 7030=3,%3d% = find_first_empty %1d% length %2d%
CLEO_Fn(FIND_FIRST_EMPTY)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);
    int length = cleo->ReadParam(handle)->i;
    int start  = cleo->ReadParam(handle)->i;

    int found = INVALID_SLOT;

    if (raw && length > 0)
    {
        uint32_t* base = reinterpret_cast<uint32_t*>(raw);
        if (start < 0) start = 0;

        for (int i = start; i < length; i++)
        {
            if (base[i] == EMPTY_SLOT)
            {
                found = i;
                break;
            }
        }
    }
    UpdateCompareFlag(handle, found != INVALID_SLOT);

    cleo->GetPointerToScriptVar(handle)->i = found;
}


// 7031=4,clear_range %1d% length %2d% start %3d% count %4d%
CLEO_Fn(CLEAR_RANGE)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);

    uint32_t* base = reinterpret_cast<uint32_t*>(raw);

    int length = cleo->ReadParam(handle)->i;
    int start  = cleo->ReadParam(handle)->i;
    int count  = cleo->ReadParam(handle)->i;

    if (length <= 0 || count <= 0) return;
    if (start < 0 || start >= length) return;

    if (start + count > length)
        count = length - start;

    for (int i = 0; i < count; i++)
        base[start + i] = EMPTY_SLOT;
}


// 7032=4,insert_at %1d% length %2d% index %3d% value %4d%
CLEO_Fn(INSERT_AT)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);

    uint32_t* base = reinterpret_cast<uint32_t*>(raw);

    int length = cleo->ReadParam(handle)->i;
    int index  = cleo->ReadParam(handle)->i;
    uint32_t value = (uint32_t)cleo->ReadParam(handle)->i;

    if (length <= 0) return;
    if (index < 0) index = 0;
    if (index >= length) return;

    for (int i = length - 1; i > index; i--)
        base[i] = base[i - 1];

    base[index] = value;
}

// 7033=3,remove_at %1d% length %2d% index %3d%
CLEO_Fn(REMOVE_AT)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);

    uint32_t* base = reinterpret_cast<uint32_t*>(raw);

    int length = cleo->ReadParam(handle)->i;
    int index  = cleo->ReadParam(handle)->i;

    if (length <= 0) return;
    if (index < 0 || index >= length) return;

    for (int i = index; i < length - 1; i++)
        base[i] = base[i + 1];

    base[length - 1] = EMPTY_SLOT;
}

// 7034=2,initialize_array_negzero %1d% length %2d%
CLEO_Fn(INITIALIZE_ARRAY_NEGZERO)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);
    int length = cleo->ReadParam(handle)->i;

    if (!raw || length <= 0) return;

    uint32_t* base = reinterpret_cast<uint32_t*>(raw);

    for (int i = 0; i < length; i++)
        base[i] = EMPTY_SLOT;
}

// 7035=3,stack_push %1d% length %2d% value %3d%
CLEO_Fn(STACK_PUSH)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);
    uint32_t* base = reinterpret_cast<uint32_t*>(raw);

    int length = cleo->ReadParam(handle)->i;
    uint32_t value = (uint32_t)cleo->ReadParam(handle)->i;

    if (!raw || length <= 0) return;

    for (int i = 0; i < length; i++)
    {
        if (base[i] == EMPTY_SLOT)
        {
            base[i] = value;
            return;
        }
    }
}

// 7036=3,%3d% = stack_pop %1d% length %2d%
CLEO_Fn(STACK_POP)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);
    int result = INVALID_SLOT;

    uint32_t* base = reinterpret_cast<uint32_t*>(raw);
    int length = cleo->ReadParam(handle)->i;

    if (length > 0)
    {
        for (int i = length - 1; i >= 0; i--)
        {
            if (base[i] != EMPTY_SLOT)
            {
                result = base[i];
                base[i] = EMPTY_SLOT;
                break;
            }
        }
    }
    
    cleo->GetPointerToScriptVar(handle)->i = result;
    
    UpdateCompareFlag(handle, result != INVALID_SLOT);
}

// 7037=3,%3d% = shift %1d% length %2d%
CLEO_Fn(SHIFT)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);
    int result = INVALID_SLOT;

    uint32_t* base = (uint32_t*)raw;
    int length = cleo->ReadParam(handle)->i;

    if (raw && length > 0)
    {
        if (base[0] != EMPTY_SLOT)
        {
            result = base[0];

            for (int i = 0; i < length - 1; i++)
                base[i] = base[i + 1];

            base[length - 1] = EMPTY_SLOT;
        }
    }
    
    cleo->GetPointerToScriptVar(handle)->i = result;
    UpdateCompareFlag(handle, result != INVALID_SLOT);
}

// 7038=4,unshift %1d% length %2d% value %3d%
CLEO_Fn(UNSHIFT)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);
    uint32_t* base = (uint32_t*)raw;
    int length = cleo->ReadParam(handle)->i;
    int value = cleo->ReadParam(handle)->i;

    if (!raw || length <= 0)
        return;

    if (base[length - 1] != EMPTY_SLOT)
        return;

    for (int i = length - 1; i > 0; i--)
        base[i] = base[i - 1];

    base[0] = value;
}

// 7039=2,reverse %1d% length %2d%
CLEO_Fn(REVERSE)
{
    void* raw = (void*)cleo->GetPointerToScriptVar(handle);
    uint32_t* base = (uint32_t*)raw;
    int length = cleo->ReadParam(handle)->i;

    if (!raw || length <= 1)
        return;

    int left = 0;
    int right = length - 1;

    while (left < right)
    {
        uint32_t tmp = base[left];
        base[left] = base[right];
        base[right] = tmp;
        left++;
        right--;
    }
}





///////////////////////////////////////////////////
//////////// END OPCODES by MatiDragon ////////////
///////////////////////////////////////////////////

void InitGrimoireOpcodes()
{
    SET_TO(TouchInterface_PositionWidgets, cleo->GetMainLibrarySymbol("_ZN15CTouchInterface10m_pWidgetsE"));

    // MatiDragon opcodes
    
    CLEO_RegisterOpcode(0x7000, SET_WIDGET_TRANSFORM); // 7000=5,set_widget_transform %1d% coords %2d% %3d% scales %4d% %5d%
    CLEO_RegisterOpcode(0x7001, GET_WIDGET_TRANSFORM); // 7001=1,get_widget_transform %1d% coords %2d% %3d% scales %4d% %5d%
    CLEO_RegisterOpcode(0x7002, LOGICAL_OR); // 7002=3,%1d% = %1d% || %2d%
    CLEO_RegisterOpcode(0x7003, FILE_RENAME); // 7003=2,file_rename %1d% to %2d%
    CLEO_RegisterOpcode(0x7004, CREATE_FILE_OR_DIRECTORY); // 7004=1,create_file_or_directory %1d%
    CLEO_RegisterOpcode(0x7005, ANGLE_DIFF); // 7005=3,%3d% = angle_diff %1d% %2d%
    CLEO_RegisterOpcode(0x7006, TOGGLE_BOOLEAN_VAR); // 7006=2,%2d% = !%1d% ; boolean
    CLEO_RegisterOpcode(0x7007, FLOAT_DIV); // 7007=3,%3d% = %1d% / %2d% ; float
    CLEO_RegisterOpcode(0x7008, FLOAT_MUL); // 7008=3,%3d% = %1d% * %2d% ; float
    CLEO_RegisterOpcode(0x7009, FLOAT_SUM); // 7009=3,%3d% = %1d% + %2d% ; float
    // 10 OPCODES ADDED
    CLEO_RegisterOpcode(0x700A, FLOAT_SUB); // 700A=3,%3d% = %1d% - %2d% ; float
    CLEO_RegisterOpcode(0x700B, SPLIT_FLOAT_TO_SIGNED_PARTS); // 700B=4,%3d% %4d% = split_float_to_signed_parts %1d% decimals %2d%
    CLEO_RegisterOpcode(0x700C, CONVERT_MODEL_COLOR); // 700C=8,%5d% %6d% %7d% = convert_model_color %1d% inputs %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x700D, IF_TERNARY_INT); // 700D=6,%6d% = int %1d% op %2d% int %3d% ? any_value %4d% : any_value %5d%
    CLEO_RegisterOpcode(0x700E, IF_TERNARY_FLOAT); // 700E=6,%6d% = float %1d% op %2d% float %3d% ? any_value %4d% : any_value %5d%
    CLEO_RegisterOpcode(0x700F, PACK_SET_BYTE); // 700F=5,%5d% = pack_set_byte %1d% byteIndex %2d% newValue %3d% isSigned %4b%
    CLEO_RegisterOpcode(0x7010, PACK_GET_BYTE); // 7010=4,%4d% = pack_get_byte %1d% byteIndex %2d% isSigned %3b%
    CLEO_RegisterOpcode(0x7011, PACK_ROTATE); // 7011=4,%4d% = pack_rotate %1d% direction %2b% amount %3d%
    CLEO_RegisterOpcode(0x7012, PACK_CHECK_TRUTHY); // 7012=4,%4d% = pack_check_truthy %1d% mask %2d% mode %3b% //IF/SET
    CLEO_RegisterOpcode(0x7013, PACK_SWAP_CUSTOM); // 7013=6,%6d% = pack_swap_custom %1d% i3 %2d% i2 %3d% i1 %4d% i0 %5d%
    // 20 OPCODES ADDED
    CLEO_RegisterOpcode(0x7014, IF_TERNARY); // 7014=4,%4d% = is_truthy %1d% ? any_value %2d% : any_value %3d%
    CLEO_RegisterOpcode(0x7015, PACK_4DEC_TO_INT32); // 7015=6,%6d% = pack_4dec_to_int32 %1d% %2d% %3d% %4d% flags %5d%
    CLEO_RegisterOpcode(0x7016, UNPACK_INT32_TO_4DEC); // 7016=6,%3d% %4d% %5d% %6d% = unpack_int32_to_4dec %1d% flags %2d%
    CLEO_RegisterOpcode(0x7017, ORBIT_CIRCLE);   // 7017=7,%6d% %7d% = orbit_circle %1b:angle/radian% angle %2d% radius %3d% coords %4d% %5d%
    CLEO_RegisterOpcode(0x7018, ORBIT_SPHERE);   // 7018=10,%8d% %9d% %10d% = orbit_sphere %1b:angle/radian% angles %2d% %3d% radius %4d% coords %5d% %6d% %7d%
    CLEO_RegisterOpcode(0x7019, ORBIT_OVAL);     // 7019=9,%8d% %9d% = orbit_oval %1b:angle/radian% angle %2d% radius %3d% %4d% rotation %5d% coords %6d% %7d%
    CLEO_RegisterOpcode(0x701A, ORBIT_OVOID);    // 701A=15,%13d% %14d% %15d% = orbit_ovoid %1b:angle/radian% angles %2d% %3d% radius %4d% %5d% %6d% rotation %7d% %8d% %9d% coords %10d% %11d% %12d%
    CLEO_RegisterOpcode(0x701B, ORBIT_CYLINDER); // 701B=15,%13d% %14d% %15d% = orbit_cylinder %1b:angle/radian% angle %2d% level %3d% height %4d% radii %5d% %6d% rotation %7d% %8d% %9d% coords %10d% %11d% %12d%
    CLEO_RegisterOpcode(0x701C, TOGGLE_BOOLEAN_REAL); // 701C=2,%2d% = !!%1d%
    CLEO_RegisterOpcode(0x701D, ORBIT_POLYGON);   // 701D=14,%12d% %13d% %14d% = orbit_polygon %1b:angle/radian% angle %2d% sides %3d% radius %4d% smooth %5d% rotation %6d% %7d% %8d% coords %9d% %10d% %11d%
    // 30 OPCODES ADDED
    CLEO_RegisterOpcode(0x701E, ORBIT_CUBE);   // 701E=16,%14d% %15d% %16d% = orbit_cube %1b:angle/radian% angles %2d% %3d% size %4d% %5d% %6d% smooth %7d% rotation %8d% %9d% %10d% coords %11d% %12d% %13d%
    CLEO_RegisterOpcode(0x701F, ORBIT_SQUARE);   // 701F=11,%9d% %10d% = orbit_square %1b:angle/radian% angle %2d% size %3d% %4d% smooth %5d% rotZ %6d% coords %7d% %8d%
    CLEO_RegisterOpcode(0x7020, ROTATE_LERP);   // 7020=6,%6d% progress %5d% = rotate_lerp %1d% %2d% dt %3d% speed %4d%
    CLEO_RegisterOpcode(0x7021, MOVE_LERP);   // 7021=12,%10d% %11d% %12d% progress %9d% = move_lerp %1d% %2d% %3d% to %4d% %5d% %6d% deltatime %7d% speed %8d%
    CLEO_RegisterOpcode(0x7022, LERP_IS_FINISHED);   // 7022=1,  lerp_is_finished %1d%
    CLEO_RegisterOpcode(0x7023, ROTATE_LERP_CURVED);   // 7023=12,%12d% progress %11d% = rotate_lerp_curved %1d% %2d% dt %3d% speed %4d% mode %5d% params %6d% %7d% %8d% %9d% overshoot %10d%
    CLEO_RegisterOpcode(0x7024, VALUE_LERP);   // 7024=6,%6d% progress %5d% = value_lerp %1d% to %2d% deltatime %3d% speed %4d%
    CLEO_RegisterOpcode(0x7025, LERP_MAINTAIN_LOOP);   // 7025=1,lerp_maintain_loop %1d%
    CLEO_RegisterOpcode(0x7026, VALUE_LERP_CURVED);   // 7026=12,%12d% progress %11d% = value_lerp_curved  %1d% to %2d% dt %3d% speed %4d% mode %5d% params %6d% %7d% %8d% %9d% overshoot %10d%
    CLEO_RegisterOpcode(0x7027, MOVE_LERP_CURVED);   // 7027=18,%16d% %17d% %18d% progress %15d% = move_lerp_curved %1d% %2d% %3d% to %4d% %5d% %6d% dt %7d% speed %8d% mode %9d% params %10d% %11d% %12d% %13d% overshoot %14d%
    // 40 OPCODES ADDED
    CLEO_RegisterOpcode(0x7028, TOGGLE_LERP_REVERSE);   // 7028=1,toggle_lerp_reverse %1d%
    CLEO_RegisterOpcode(0x7029, QUADRATIC_LERP_CURVED);   // 7029=21,%19d% %20d% %21d% progress %18d% = quadratic_lerp_curved %1d% %2d% %3d% per %4d% %5d% %6d% to %7d% %8d% %9d% dt %10d% speed %11d% mode %12d% params %13d% %14d% %15d% %16d% overshoot %17d%
    CLEO_RegisterOpcode(0x702A, BLEND_VEC3);   // 702A=10,%8d% %9d% %10d% = blend_vec3 %1d% %2d% %3d% to %4d% %5d% %6d% blend %7d%
    CLEO_RegisterOpcode(0x702B, BLEND_VEC2);   // 702B=7,%6d% %7d% = blend_vec2 %1d% %2d% to %3d% %4d% blend %5d%
    CLEO_RegisterOpcode(0x702C, IS_LERP_OVERSHOOTING);   // 702C=1,  is_lerp_overshooting %1d%
    CLEO_RegisterOpcode(0x702D, WRITE_ARRAY_SAFE);   // 702D=4,write_array_safe %1d% length %2d% index %3d% value %4d%
    CLEO_RegisterOpcode(0x702E, READ_ARRAY_SAFE);   // 702E=4,%4d% = read_array_safe %1d% length %2d% index %3d%
    CLEO_RegisterOpcode(0x702F, COUNT_ARRAY_SLOTS);   // 702F=4,count_array_slots %1d% length %2d% empty %3d% used %4d%
    CLEO_RegisterOpcode(0x7030, FIND_FIRST_EMPTY);   // 7030=3,%3d% = find_first_empty %1d% length %2d%
    CLEO_RegisterOpcode(0x7031, CLEAR_RANGE);   // 7031=4,clear_range %1d% length %2d% start %3d% count %4d%
    // 50 OPCODES ADDED
    CLEO_RegisterOpcode(0x7032, INSERT_AT);   // 7032=4,insert_at %1d% length %2d% index %3d% value %4d%
    CLEO_RegisterOpcode(0x7033, REMOVE_AT);   // 7033=3,remove_at %1d% length %2d% index %3d%
    CLEO_RegisterOpcode(0x7034, INITIALIZE_ARRAY_NEGZERO);   // 7034=2,initialize_array_negzero %1d% length %2d%
    CLEO_RegisterOpcode(0x7035, STACK_PUSH);   // 7035=3,stack_push %1d% length %2d% value %3d%
    CLEO_RegisterOpcode(0x7036, STACK_POP);   // 7036=3,%3d% = stack_pop %1d% length %2d%
    CLEO_RegisterOpcode(0x7037, SHIFT);   // 7037=3,%3d% = shift %1d% length %2d%
    CLEO_RegisterOpcode(0x7038, UNSHIFT);   // 7038=4,unshift %1d% length %2d% value %3d%
    CLEO_RegisterOpcode(0x7039, REVERSE);   // 7039=2,reverse %1d% length %2d%
}