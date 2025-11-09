#include <mod/amlmod.h>
#include <mod/logger.h>
#include <cleohelpers.h>

#include <dirent.h>
#include <list>
#include <string>
#include <sys/stat.h>

#include <math.h>
/////////////////////////////////////////////////////
/////////// BEGIN OPCODES by MatiDragon /////////////
/////////////////////////////////////////////////////

/*
CLEO_Fn(SET_WIDGET_TRANSFORM)
{
    // Widget ID
    int buttonId = cleo->ReadParam(handle)->i;

    // Transform
    float x = cleo->ReadParam(handle)->f;      // Coord X
    float y = cleo->ReadParam(handle)->f;      // Coord Y
    float width = cleo->ReadParam(handle)->f;  // Width
    float height = cleo->ReadParam(handle)->f; // Height

    uintptr_t widgetsAddr = (*TouchInterfaceWidgets);

    // Calculate button address
    widgetsAddr += buttonId * 4;
    uintptr_t buttonPtr = *(uintptr_t*)widgetsAddr;

    if (buttonPtr)
    {
        buttonPtr += 12; // Offset to access position
        *(float*)buttonPtr = x;      // Write X position
        buttonPtr += 4;
        *(float*)buttonPtr = y;      // Write Y position
        buttonPtr += 4;
        *(float*)buttonPtr = width;  // Write width
        buttonPtr += 4;
        *(float*)buttonPtr = height; // Write height
    }

    UpdateCompareFlag(handle, buttonPtr != 0); // Update compare flag
}

CLEO_Fn(IS_TOUCH_PRESSED)
{
    uintptr_t touchDownAddr = cleo->TouchInterfaceTouchDown();

    uint8_t isPressed = *(uint8_t*)touchDownAddr; // Read touch state

    // Update compare flag
    UpdateCompareFlag(handle, isPressed != 0);

    // Optional return: if the script requests to store the value
    if (GetVarArgCount(handle) > 0)
    {
        cleo->GetPointerToScriptVar(handle)->i = isPressed;
    }
}

CLEO_Fn(GET_TOUCH_XY)
{
    uintptr_t touchPosAddr = cleo->TouchInterfaceCachedPos();

    float x = *(float*)touchPosAddr;       // X
    float y = *(float*)(touchPosAddr + 4); // Y

    cleo->GetPointerToScriptVar(handle)->i = (int)x; // Convert to integer
    cleo->GetPointerToScriptVar(handle)->i = (int)y; // Convert to integer
}
*/
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
    int decimals = cleo->ReadParam(handle)->i;

    // Clamp decimals to [0,9] to avoid overflow on large multipliers
    if (decimals < 0) decimals = 0;
    if (decimals > 9) decimals = 9;

    // Sign and absolute value
    int sign = (v < 0.0f) ? -1 : 1;
    float absv = fabsf(v);

    // Integer part (trunc toward zero)
    int intPart = (int)absv;
    intPart *= sign; // restore sign for integer part

    // Fractional part: take absolute fractional, scale, then truncate (no rounding)
    float frac = absv - (float)((int)absv);
    int multiplier = 1;
    for (int i = 0; i < decimals; ++i) multiplier *= 10;

    int fracPart = 0;
    if (multiplier > 1)
    {
        fracPart = (int)(frac * (float)multiplier); // truncates toward zero
    }
    else
    {
        fracPart = 0;
    }

    fracPart *= sign; // make fraction share the sign of original value

    // Return values: first int (integer part), second int (fractional scaled)
    cleo->GetPointerToScriptVar(handle)->i = intPart;
    cleo->GetPointerToScriptVar(handle)->i = fracPart;

    UpdateCompareFlag(handle, true);
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

// Helpers clamp
static float clampf(float v, float a, float b) { if (v < a) return a; if (v > b) return b; return v; }
static int clampi(int v, int a, int b) { if (v < a) return a; if (v > b) return b; return v; }

// RGB(0..255) -> HSV(H:0..360 int, S:0..100 int, V:0..100 int)
static void RGB_to_HSV_int_scale(int r, int g, int b, int &outH, int &outS, int &outV)
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
static void HSV_to_RGB_int_scale(int H, int S, int V, int &outR, int &outG, int &outB)
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
static void RGB_to_HSL_int_scale(int r, int g, int b, int &outH, int &outS, int &outL)
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
static void HSL_to_RGB_int_scale(int H, int S, int L, int &outR, int &outG, int &outB)
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

// ---------------- CLEO opcodes: ALPHA OBLIGATORIA --------------------

// Firma obligatoria: todos reciben y devuelven A (0..255).

// r g b a = CONV_RGB_TO_HSV_INT r g b a
CLEO_Fn(CONV_RGB_TO_HSV_INT)
{
    int r = cleo->ReadParam(handle)->i;
    int g = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    int a = cleo->ReadParam(handle)->i; // OBLIGATORIO

    int H,S,V;
    RGB_to_HSV_int_scale(r,g,b,H,S,V);

    cleo->GetPointerToScriptVar(handle)->i = H;
    cleo->GetPointerToScriptVar(handle)->i = S;
    cleo->GetPointerToScriptVar(handle)->i = V;
    cleo->GetPointerToScriptVar(handle)->i = a;
}

// r g b a = CONV_HSV_TO_RGB_INT H S V A
CLEO_Fn(CONV_HSV_TO_RGB_INT)
{
    int H = cleo->ReadParam(handle)->i;
    int S = cleo->ReadParam(handle)->i;
    int V = cleo->ReadParam(handle)->i;
    int a = cleo->ReadParam(handle)->i; // OBLIGATORIO

    int r,g,b;
    HSV_to_RGB_int_scale(H,S,V,r,g,b);

    cleo->GetPointerToScriptVar(handle)->i = r;
    cleo->GetPointerToScriptVar(handle)->i = g;
    cleo->GetPointerToScriptVar(handle)->i = b;
    cleo->GetPointerToScriptVar(handle)->i = a;
}

// H S L a = CONV_RGB_TO_HSL_INT r g b a
CLEO_Fn(CONV_RGB_TO_HSL_INT)
{
    int r = cleo->ReadParam(handle)->i;
    int g = cleo->ReadParam(handle)->i;
    int b = cleo->ReadParam(handle)->i;
    int a = cleo->ReadParam(handle)->i; // OBLIGATORIO

    int H,S,L;
    RGB_to_HSL_int_scale(r,g,b,H,S,L);

    cleo->GetPointerToScriptVar(handle)->i = H;
    cleo->GetPointerToScriptVar(handle)->i = S;
    cleo->GetPointerToScriptVar(handle)->i = L;
    cleo->GetPointerToScriptVar(handle)->i = a;
}

// r g b a = CONV_HSL_TO_RGB_INT H S L A
CLEO_Fn(CONV_HSL_TO_RGB_INT)
{
    int H = cleo->ReadParam(handle)->i;
    int S = cleo->ReadParam(handle)->i;
    int L = cleo->ReadParam(handle)->i;
    int a = cleo->ReadParam(handle)->i; // OBLIGATORIO

    int r,g,b;
    HSL_to_RGB_int_scale(H,S,L,r,g,b);

    cleo->GetPointerToScriptVar(handle)->i = r;
    cleo->GetPointerToScriptVar(handle)->i = g;
    cleo->GetPointerToScriptVar(handle)->i = b;
    cleo->GetPointerToScriptVar(handle)->i = a;
}

// Helpers (float)
static inline float DegToRadF(float deg) { return deg * (3.14159265358979323846f / 180.0f); }
static inline bool IsFiniteFloat(float v) { return finite(v); }

// ORBIT_2D
CLEO_Fn(ORBIT_2D)
{
    int angleMode = cleo->ReadParam(handle)->i;
    float angle = cleo->ReadParam(handle)->f;
    float radius = cleo->ReadParam(handle)->f;
    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;

    if (angleMode == 0) angle = DegToRadF(angle);

    if (!IsFiniteFloat(angle) || !IsFiniteFloat(radius) || !IsFiniteFloat(cx) || !IsFiniteFloat(cy))
    {
        UpdateCompareFlag(handle, false);
        return;
    }

    float x = cosf(angle) * radius + cx;
    float y = sinf(angle) * radius + cy;

    cleo->GetPointerToScriptVar(handle)->f = x;
    cleo->GetPointerToScriptVar(handle)->f = y;

    UpdateCompareFlag(handle, true);
}

// ORBIT_3D
CLEO_Fn(ORBIT_3D)
{
    int angleMode = cleo->ReadParam(handle)->i;
    float ax = cleo->ReadParam(handle)->f;
    float ay = cleo->ReadParam(handle)->f;
    float radius = cleo->ReadParam(handle)->f;
    float cx = cleo->ReadParam(handle)->f;
    float cy = cleo->ReadParam(handle)->f;
    float cz = cleo->ReadParam(handle)->f;

    if (angleMode == 0) { ax = DegToRadF(ax); ay = DegToRadF(ay); }

    if (!IsFiniteFloat(ax) || !IsFiniteFloat(ay) || !IsFiniteFloat(radius) ||
        !IsFiniteFloat(cx) || !IsFiniteFloat(cy) || !IsFiniteFloat(cz))
    {
        UpdateCompareFlag(handle, false);
        return;
    }

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

    UpdateCompareFlag(handle, true);
}



///////////////////////////////////////////////////
//////////// END OPCODES by MatiDragon ////////////
///////////////////////////////////////////////////

void InitUtilsOpcodes()
{
    // MatiDragon opcodes
    //CLEO_RegisterOpcode(0x7000, SET_WIDGET_TRANSFORM); // 7000=5,set_widget_transform %1d% coords %2d% %3d% scales %4d% %5d%
    //CLEO_RegisterOpcode(0x7001, IS_TOUCH_PRESSED); // 7001=1,is_touch_pressed store_to %1d%
    //CLEO_RegisterOpcode(0x7002, GET_TOUCH_XY); // 7002=2,get_touch_xy %1d% %2d%
    //
    // CLEO_RegisterOpcode(0x7002, DELETE_FILE_OR_DIRECTORY); // 7002=1,delete_file_or_directory %1d%
    CLEO_RegisterOpcode(0x7003, CREATE_FILE_OR_DIRECTORY); // 7003=1,create_file_or_directory %1d%
    CLEO_RegisterOpcode(0x7005, ANGLE_DIFF); // 7005=3,%3d% = angle_diff %1d% %2d%
    CLEO_RegisterOpcode(0x7006, TOGGLE_BOOLEAN_VAR); // 7006=2,%2d% = !%1d% ; boolean
    CLEO_RegisterOpcode(0x7007, FLOAT_DIV); // 7007=3,%3d% = %1d% / %2d% ; float
    CLEO_RegisterOpcode(0x7008, FLOAT_MUL); // 7008=3,%3d% = %1d% * %2d% ; float
    CLEO_RegisterOpcode(0x7009, FLOAT_SUM); // 7009=3,%3d% = %1d% + %2d% ; float
    CLEO_RegisterOpcode(0x700A, FLOAT_SUB); // 700A=3,%3d% = %1d% - %2d% ; float
    CLEO_RegisterOpcode(0x700B, SPLIT_FLOAT_TO_SIGNED_PARTS); // 700B=4,%3d% %4d% = split_float_to_signed_parts %1d% decimals %2d%
    CLEO_RegisterOpcode(0x700C, FILE_RENAME); // 700C=2,file_rename %1d% to %2d%
    CLEO_RegisterOpcode(0x700D, CONV_RGB_TO_HSV_INT); // 700D=8,%5d% %6d% %7d% %8d% = CONV_RGB_TO_HSV_INT %1d% %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x700E, CONV_HSV_TO_RGB_INT); // 700E=8,%5d% %6d% %7d% %8d% = CONV_HSV_TO_RGB_INT %1d% %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x700F, CONV_RGB_TO_HSL_INT); // 700F=8,%5d% %6d% %7d% %8d% = CONV_RGB_TO_HSL_INT %1d% %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x7010, CONV_HSL_TO_RGB_INT); // 7010=8,%5d% %6d% %7d% %8d% = CONV_HSL_TO_RGB_INT %1d% %2d% %3d% %4d%
    CLEO_RegisterOpcode(0x7017, ORBIT_2D); // 7017=7,ORBIT_2D %6d% %7d% = angleMode %1d% angle %2d% radius %3d% cx %4d% cy %5d%
    CLEO_RegisterOpcode(0x7018, ORBIT_3D); // 7018=10,ORBIT_3D %8d% %9d% %10d% = angleMode %1d% ax %2d% ay %3d% radius %4d% cx %5d% cy %6d% cz %7d%
}