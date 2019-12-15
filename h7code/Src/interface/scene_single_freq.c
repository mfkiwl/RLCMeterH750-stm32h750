#include "scene_single_freq.h"
#include "task.h"
#include "srlc_format.h"
#include "scene_single_freq_menu.h"
#include "hardware/select_resistor.h"
#include "measure/calculate_rc.h"
#include <stdio.h>

static void SceneSingleFreqQuant();
static void SceneSingleFreqDrawFreq();
static void SceneSingleFreqDrawNames();
static void SceneSingleFreqDrawValues();
static void SceneSingleFreqDrawCurrentR();
static void SceneSingleFreqDrawDebug();

static int freq_x;
static int freq_y;
static int freq_y_max;
static int freq_width;

static const char* s_g_single_freq_khz = " Hz";
#define FONT_OFFSET_30TO59 24

static int pb_name_width;
static int pb_param_width;
static int pb_param_x;
static int pb_param1_name_y;
static int pb_param2_name_y;
static int pb_param1_value_y;
static int pb_param2_value_y;
static int pb_param_x_type;
static int pb_param_width_type;

bool view_parallel = false;
bool view_LC = true;
bool view_debug = false;

static complex last_Zx;
static bool last_Zx_changed;

static int info_current_r_x;
static int info_current_r_y;
static int info_current_r_width;
ResistorSelectorEnum last_current_r;

static const uint16_t REAL_BACK_COLOR = VGA_MAROON;
static const uint16_t IMAG_BACK_COLOR = VGA_TEAL;


//Предполагается, что str, это строчка у которой может быть - вначале
//Если минуса нет, то оставляем под него пустое место.
int DrawNumberMinus(int x, int y, const char* str, int width)
{
    if(str[0]=='-')
    {
        return UTF_DrawStringJustify(x, y, str, width, UTF_LEFT);
    }

    int minus_width = UTF_StringWidth("-");
    UTFT_fillRectBack(x, y, x+minus_width-1, y+UTF_Height()-1);

    return UTF_DrawStringJustify(x+minus_width, y, str, width-minus_width, UTF_LEFT);
}

void DrawNumberType(int x, int y, const char* str_number, const char* str_type)
{
    UTF_SetFont(font_condensed59);
    int height_big = UTF_Height();

    UTF_DrawStringJustify(x, y, str_number, pb_param_width, UTF_RIGHT);

    UTF_SetFont(font_condensed30);
    int height_small = UTF_Height();
    int yadd = FONT_OFFSET_30TO59;
    int x1 = pb_param_x_type;
    int x2 = UTF_DrawStringJustify(x1, y+yadd, str_type, pb_param_width_type, UTF_LEFT);

    UTFT_fillRectBack(x1, y, x2-1, y+yadd-1);
    UTFT_fillRectBack(x1, y+yadd+height_small, x2-1, y+height_big-1);
}

void SceneSingleFreqStart()
{
    last_Zx = 0;
    last_Zx_changed = false;

    int y;
    UTFT_setColorW(VGA_WHITE);
    UTF_SetFont(font_condensed30);
    y = 0;
    freq_y = y;
    y += UTF_Height();
    freq_y_max = y;

    UTFT_setBackColorW(COLOR_BACKGROUND_BLUE);
    UTFT_fillRectBack(0, 0, UTFT_getDisplayXSize()-1, freq_y_max-1);
    UTFT_setBackColorW(VGA_BLACK);
    UTFT_fillRectBack(0, freq_y_max, UTFT_getDisplayXSize()-1, UTFT_getDisplayYSize()-1);

    {
        //Рассчитываем центр строки 000000 Hz
        int width = 0;
        UTF_SetFont(font_condensed30);
        width +=  UTF_StringWidth(s_g_single_freq_khz);
        freq_width = UTF_StringWidth("000000");
        width += freq_width;

        freq_x = UTFT_getDisplayXSize()-width;
    }

    UTFT_setBackColorW(COLOR_BACKGROUND_BLUE);
    UTF_SetFont(font_condensed30);
    UTF_DrawString(freq_x+freq_width, freq_y, s_g_single_freq_khz);
    UTF_DrawStringJustify(0, freq_y, "RLCMeterH7", freq_x, UTF_CENTER);

    UTF_SetFont(font_condensed59);
    pb_param_width = UTF_StringWidth("-00.000");
    UTF_SetFont(font_condensed30);
    pb_param_width_type = UTF_StringWidth(" MOm");

    pb_name_width = pb_param_width+pb_param_width_type;
    pb_param_x = (UTFT_getDisplayXSize()-pb_name_width)/2;

    pb_param_x_type = pb_param_x+pb_param_width;

    UTF_SetFont(font_condensed30);
    pb_param1_name_y = y;
    y += UTF_Height();
    UTF_SetFont(font_condensed59);
    pb_param1_value_y = y;
    y += UTF_Height();
    UTF_SetFont(font_condensed30);
    pb_param2_name_y = y;
    y += UTF_Height();
    UTF_SetFont(font_condensed59);
    pb_param2_value_y = y;
    y += UTF_Height();


    UTF_SetFont(font_condensed30);
    info_current_r_x = 0;
    info_current_r_y = UTFT_getDisplayYSize()-UTF_Height();
    info_current_r_width = UTF_StringWidth("Rc=10 KOm");

    UTF_DrawString(info_current_r_x + info_current_r_width+10, info_current_r_y, correctionValid()?"valid":"inval");

    SceneSingleFreqDrawFreq();
    SceneSingleFreqDrawNames();
    SceneSingleFreqDrawCurrentR();
    InterfaceGoto(SceneSingleFreqQuant);
}

void SceneSingleFreqQuant()
{
    if(EncValueChanged())
    {
        AddSaturated(&g_freq_index, EncValueDelta(), FREQ_INDEX_MAX);
        TaskSetFreq(StandartFreq(g_freq_index));
        SceneSingleFreqDrawFreq();
    }

    if(EncButtonPressed())
    {
        SceneSingleFreqMenuStart();
        return;
    }

    if(last_Zx_changed)
    {
        last_Zx_changed = false;
        if(view_debug)
            SceneSingleFreqDrawDebug();
        else
            SceneSingleFreqDrawValues();
    }

    if(ResistorCurrent()!=last_current_r)
        SceneSingleFreqDrawCurrentR();
}

void SceneSingleFreqZx(complex Zx)
{
    if(!InterfaceIsActive(SceneSingleFreqQuant))
        return;
    last_Zx = Zx;
    last_Zx_changed = true;
}

void SceneSingleFreqDrawFreq()
{
    UTF_SetFont(font_condensed30);
    UTFT_setColorW(VGA_WHITE);
    UTFT_setBackColorW(COLOR_BACKGROUND_BLUE);
    UTF_printNumI(TaskGetFreq(), freq_x, freq_y, freq_width, UTF_RIGHT);
}

void SceneSingleFreqDrawNames()
{
    UTF_SetFont(font_condensed30);
    UTFT_setColorW(VGA_WHITE);

    char* str_re = "real(Z)";
    char* str_im = "imag(Z)";

    if(view_LC)
    {
        str_re = view_parallel?"EPR":"ESR";
        str_im = "L/C";
    }

    UTFT_setBackColorW(REAL_BACK_COLOR);
    UTF_DrawStringJustify(pb_param_x, pb_param1_name_y, str_re, pb_name_width, UTF_CENTER);
    UTFT_setBackColorW(IMAG_BACK_COLOR);
    UTF_DrawStringJustify(pb_param_x, pb_param2_name_y, str_im, pb_name_width, UTF_CENTER);
}

void SceneSingleFreqDrawValues()
{
    const int outstr_size = 10;
    char str_re[outstr_size];
    char str_im[outstr_size];
    char str_re_type[outstr_size];
    char str_im_type[outstr_size];

    UTFT_setColorW(VGA_WHITE);
    UTFT_setBackColorW(COLOR_BACKGROUND_BLUE);

    static VisualInfo info;
    convertZxmToVisualInfo(last_Zx, TaskGetFreq(), view_parallel, &info);

    float Rabs = cabsf(last_Zx);
    formatR2(str_re, str_re_type, info.Rre, view_parallel?fabsf(info.Rre):Rabs);

    if(view_LC)
    {
        if(info.is_inductance)
            formatL2(str_im, str_im_type, info.L);
        else
            formatC2(str_im, str_im_type, info.C);
    } else
    {
        formatR2(str_im, str_im_type, info.Rim, view_parallel?fabsf(info.Rim):Rabs);
    }

    //formatR2(str_re, str_re_type, crealf(last_Zx), Rabs);
    //formatR2(str_im, str_im_type, cimagf(last_Zx), Rabs);

    UTFT_setBackColorW(REAL_BACK_COLOR);
    DrawNumberType(pb_param_x, pb_param1_value_y, str_re, str_re_type);
    UTFT_setBackColorW(IMAG_BACK_COLOR);
    DrawNumberType(pb_param_x, pb_param2_value_y, str_im, str_im_type);
}

void SceneSingleFreqDrawCurrentR()
{
    last_current_r = ResistorCurrent();

    char* str_r = "Rc=100 Om";
    if(last_current_r == Resistor_1_KOm)
        str_r = "Rc=1 KOm";
    if(last_current_r == Resistor_10_KOm)
        str_r = "Rc=10 KOm";

    UTF_SetFont(font_condensed30);
    UTFT_setColorW(VGA_WHITE);
    UTFT_setBackColorW(COLOR_BACKGROUND_BLUE);
    UTF_DrawStringJustify(info_current_r_x, info_current_r_y, str_r, info_current_r_width, UTF_RIGHT);
}

void SceneSingleFreqDrawDebug()
{
    char buf[32];
    UTF_SetFont(font_condensed30);
    UTFT_setColorW(VGA_WHITE);
    int y;

    UTFT_setBackColorW(REAL_BACK_COLOR);

    y = pb_param1_name_y;
    strcpy(buf, "abs(a)=");
    floatToString(buf+strlen(buf), 20, cabs(g_result.sum_a), 1, 7, false);
    UTF_DrawStringJustify(pb_param_x, y, buf, pb_name_width, UTF_CENTER);

    y += UTF_Height();
    sprintf(buf, "mid_a=%i", (int)g_result.mid_a);
    UTF_DrawStringJustify(pb_param_x, y, buf, pb_name_width, UTF_CENTER);

    y += UTF_Height();
    strcpy(buf, "abs(Zxm)=");
    floatToString(buf+strlen(buf), 20, cabs(g_Zxm), 1, 7, false);
    UTF_DrawStringJustify(pb_param_x, y, buf, pb_name_width, UTF_CENTER);

    UTFT_setBackColorW(IMAG_BACK_COLOR);

    y = pb_param2_name_y;
    strcpy(buf, "abs(b)=");
    floatToString(buf+strlen(buf), 20, cabs(g_result.sum_b), 1, 7, false);
    UTF_DrawStringJustify(pb_param_x, y, buf, pb_name_width, UTF_CENTER);

    y += UTF_Height();
    sprintf(buf, "mid_b=%i", (int)g_result.mid_b);
    UTF_DrawStringJustify(pb_param_x, y, buf, pb_name_width, UTF_CENTER);

}
