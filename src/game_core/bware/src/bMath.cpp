#include "bware/inc/bMath.hpp"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

// These entry points retain existing caller ABI, but perform native CPU
// arithmetic. The PS2 mantissa/carry/divider/root state machines were
// removed from the PC runtime completely.
float bScalarAdd(float left, float right) { return left + right; }
float bScalarMultiply(float left, float right) { return left * right; }
float bScalarDivide(float numerator, float denominator) { return numerator / denominator; }
float bScalarSquareRoot(float value) { return sqrtf(value); }

// Preserve the PC game's explicit guard for invalid physical ratios, not a
// console floating-point implementation. This retains finite zero handling.
float bScalarFiniteRatio(float numerator, float denominator)
{
    return finiteRatio(numerator, denominator);
}

uint32_t vectorIntegerSquareRoot(uint32_t value)
{
    uint32_t doubledRoot = 0, remainder = 0;
    for (int pair = 0; pair < 16; ++pair) {
        remainder = (remainder << 2) + (value >> 30);
        doubledRoot <<= 1;
        if (doubledRoot < remainder) {
            remainder = remainder - doubledRoot - 1;
            doubledRoot += 2;
        }
        value <<= 2;
    }
    return doubledRoot >> 1;
}
float vectorWrapFloat(float value, float period)
{
    const float width = fabsf(period);
    const float coordinate = finiteRatio(value, width);
    float integer = (float)(int32_t)coordinate;
    if (coordinate < integer) integer = integer - 1.0f;
    const float fraction = coordinate - integer;
    return fraction * width;
}

typedef struct ArcSample { uint16_t angle; float slope; } ArcSample;

static const ArcSample arcSamples[209] = {
    {0, 0.15919999778270721f},
    {326, 0.15929999947547913f},
    {652, 0.15960000455379486f},
    {979, 0.16009999811649323f},
    {1307, 0.1607999950647354f},
    {1636, 0.1615999937057495f},
    {1967, 0.16249999403953552f},
    {2300, 0.16369999945163727f},
    {2635, 0.16509999334812164f},
    {2973, 0.16670000553131104f},
    {3315, 0.16850000619888306f},
    {3660, 0.1704999953508377f},
    {4009, 0.1729000061750412f},
    {4363, 0.17560000717639923f},
    {4723, 0.1784999966621399f},
    {5088, 0.1818999946117401f},
    {5461, 0.18479999899864197f},
    {5650, 0.1868000030517578f},
    {5841, 0.1889999955892563f},
    {6035, 0.19130000472068787f},
    {6231, 0.19380000233650208f},
    {6429, 0.1964000016450882f},
    {6630, 0.19920000433921814f},
    {6834, 0.20229999721050262f},
    {7041, 0.20559999346733093f},
    {7252, 0.20909999310970306f},
    {7466, 0.21289999783039093f},
    {7684, 0.21699999272823334f},
    {7906, 0.2214999943971634f},
    {8133, 0.22630000114440918f},
    {8365, 0.23160000145435333f},
    {8602, 0.23749999701976776f},
    {8845, 0.24220000207424164f},
    {8969, 0.24570000171661377f},
    {9095, 0.2492000013589859f},
    {9223, 0.2529999911785126f},
    {9352, 0.25699999928474426f},
    {9484, 0.2612000107765198f},
    {9617, 0.26570001244544983f},
    {9753, 0.2705000042915344f},
    {9892, 0.27559998631477356f},
    {10033, 0.28110000491142273f},
    {10177, 0.28690001368522644f},
    {10324, 0.29319998621940613f},
    {10474, 0.30000001192092896f},
    {10628, 0.3073999881744385f},
    {10785, 0.31540000438690186f},
    {10946, 0.32409998774528503f},
    {11112, 0.3312000036239624f},
    {11197, 0.3361999988555908f},
    {11283, 0.34150001406669617f},
    {11371, 0.34709998965263367f},
    {11460, 0.3529999852180481f},
    {11550, 0.35920000076293945f},
    {11642, 0.36579999327659607f},
    {11736, 0.37279999256134033f},
    {11831, 0.38029998540878296f},
    {11928, 0.3882000148296356f},
    {12028, 0.3968000113964081f},
    {12129, 0.4059000015258789f},
    {12233, 0.415800005197525f},
    {12340, 0.42640000581741333f},
    {12449, 0.43799999356269836f},
    {12561, 0.4507000148296356f},
    {12676, 0.4609000086784363f},
    {12735, 0.46810001134872437f},
    {12795, 0.4758000075817108f},
    {12856, 0.4837999939918518f},
    {12918, 0.49230000376701355f},
    {12981, 0.5012000203132629f},
    {13045, 0.510699987411499f},
    {13111, 0.520799994468689f},
    {13177, 0.5314000248908997f},
    {13245, 0.542900025844574f},
    {13315, 0.5551000237464905f},
    {13386, 0.5681999921798706f},
    {13459, 0.5823000073432922f},
    {13533, 0.5975000262260437f},
    {13610, 0.6140000224113464f},
    {13688, 0.632099986076355f},
    {13769, 0.6467000246047974f},
    {13810, 0.6570000052452087f},
    {13853, 0.667900025844574f},
    {13895, 0.6793000102043152f},
    {13939, 0.6913999915122986f},
    {13983, 0.704200029373169f},
    {14028, 0.7177000045776367f},
    {14074, 0.7319999933242798f},
    {14121, 0.7472000122070312f},
    {14169, 0.7634999752044678f},
    {14218, 0.7807999849319458f},
    {14267, 0.7994999885559082f},
    {14319, 0.8195000290870667f},
    {14371, 0.8411999940872192f},
    {14425, 0.8647000193595886f},
    {14480, 0.8902999758720398f},
    {14537, 0.9110000133514404f},
    {14566, 0.9257000088691711f},
    {14596, 0.941100001335144f},
    {14626, 0.9574000239372253f},
    {14657, 0.9745000004768372f},
    {14688, 0.9926000237464905f},
    {14720, 1.0117000341415405f},
    {14752, 1.032099962234497f},
    {14785, 1.0536999702453613f},
    {14819, 1.07669997215271f},
    {14853, 1.1013000011444092f},
    {14889, 1.1276999711990356f},
    {14925, 1.1562000513076782f},
    {14962, 1.186900019645691f},
    {15000, 1.2201999425888062f},
    {15039, 1.256500005722046f},
    {15079, 1.2857999801635742f},
    {15099, 1.3065999746322632f},
    {15120, 1.3285000324249268f},
    {15142, 1.3515000343322754f},
    {15163, 1.3758000135421753f},
    {15185, 1.4014999866485596f},
    {15208, 1.4285999536514282f},
    {15231, 1.4573999643325806f},
    {15254, 1.4880000352859497f},
    {15278, 1.5205999612808228f},
    {15302, 1.5555000305175781f},
    {15327, 1.592900037765503f},
    {15352, 1.6331000328063965f},
    {15378, 1.6765999794006348f},
    {15405, 1.7237000465393066f},
    {15433, 1.7750999927520752f},
    {15461, 1.8166999816894531f},
    {15476, 1.8460999727249146f},
    {15491, 1.8770999908447266f},
    {15506, 1.9097000360488892f},
    {15521, 1.944100022315979f},
    {15536, 1.9803999662399292f},
    {15552, 2.0188000202178955f},
    {15568, 2.059499979019165f},
    {15585, 2.102799892425537f},
    {15602, 2.1489999294281006f},
    {15619, 2.1982998847961426f},
    {15636, 2.251300096511841f},
    {15655, 2.308199882507324f},
    {15673, 2.3696999549865723f},
    {15692, 2.4363999366760254f},
    {15711, 2.5090999603271484f},
    {15731, 2.5678999423980713f},
    {15742, 2.609600067138672f},
    {15752, 2.6535000801086426f},
    {15763, 2.6995999813079834f},
    {15774, 2.748199939727783f},
    {15785, 2.799499988555908f},
    {15796, 2.8538999557495117f},
    {15807, 2.9114999771118164f},
    {15819, 2.9728000164031982f},
    {15831, 3.038100004196167f},
    {15843, 3.1078999042510986f},
    {15855, 3.182800054550171f},
    {15868, 3.2632999420166016f},
    {15881, 3.350399971008301f},
    {15895, 3.444700002670288f},
    {15908, 3.547600030899048f},
    {15923, 3.63070011138916f},
    {15930, 3.68969988822937f},
    {15937, 3.751699924468994f},
    {15945, 3.816999912261963f},
    {15952, 3.885699987411499f},
    {15960, 3.9583001136779785f},
    {15968, 4.035200119018555f},
    {15976, 4.116700172424316f},
    {15984, 4.203400135040283f},
    {15993, 4.29580020904541f},
    {16001, 4.394499778747559f},
    {16010, 4.500400066375732f},
    {16019, 4.6143999099731445f},
    {16028, 4.737400054931641f},
    {16038, 4.8709001541137695f},
    {16048, 5.01639986038208f},
    {16058, 5.133999824523926f},
    {16063, 5.21750020980835f},
    {16068, 5.305200099945068f},
    {16073, 5.39739990234375f},
    {16079, 5.494699954986572f},
    {16084, 5.597400188446045f},
    {16090, 5.706099987030029f},
    {16095, 5.821400165557861f},
    {16101, 5.943900108337402f},
    {16107, 6.0746002197265625f},
    {16113, 6.214300155639648f},
    {16119, 6.364099979400635f},
    {16126, 6.525199890136719f},
    {16132, 6.6992998123168945f},
    {16139, 6.8881001472473145f},
    {16146, 7.093800067901611f},
    {16153, 7.319200038909912f},
    {16160, 7.567399978637695f},
    {16168, 7.842899799346924f},
    {16176, 8.150699615478516f},
    {16184, 8.498000144958496f},
    {16192, 8.893899917602539f},
    {16201, 9.350799560546875f},
    {16211, 9.88640022277832f},
    {16221, 10.526100158691406f},
    {16231, 11.308899879455566f},
    {16242, 12.297699928283691f},
    {16255, 13.602499961853027f},
    {16268, 15.439499855041504f},
    {16284, 18.31399917602539f},
    {16302, 23.86720085144043f},
    {16326, 57.62030029296875f},
    {16383, -0.0003000000142492354f},
};

static const int16_t fixedSineSamples[513] = {
    0, 402, 804, 1206, 1607, 2009, 2410, 2811, 3211, 3611, 4011, 4409,
    4807, 5205, 5601, 5997, 6392, 6786, 7179, 7571, 7961, 8351, 8739, 9126,
    9511, 9895, 10278, 10659, 11038, 11416, 11792, 12166, 12539, 12909, 13278, 13645,
    14009, 14372, 14732, 15090, 15446, 15799, 16150, 16499, 16845, 17189, 17530, 17868,
    18204, 18537, 18867, 19194, 19519, 19840, 20159, 20474, 20787, 21096, 21402, 21705,
    22005, 22301, 22594, 22883, 23169, 23452, 23731, 24007, 24278, 24547, 24811, 25072,
    25329, 25582, 25831, 26077, 26318, 26556, 26790, 27019, 27245, 27466, 27683, 27896,
    28105, 28310, 28510, 28706, 28898, 29085, 29268, 29447, 29621, 29791, 29956, 30117,
    30273, 30424, 30571, 30714, 30852, 30985, 31113, 31237, 31356, 31471, 31580, 31685,
    31785, 31880, 31971, 32057, 32138, 32214, 32285, 32351, 32413, 32469, 32521, 32568,
    32610, 32647, 32679, 32706, 32728, 32745, 32758, 32765, 32767, 32765, 32758, 32745,
    32728, 32706, 32679, 32647, 32610, 32568, 32521, 32470, 32413, 32352, 32285, 32214,
    32138, 32057, 31972, 31881, 31786, 31686, 31581, 31471, 31357, 31238, 31114, 30986,
    30853, 30715, 30572, 30425, 30274, 30118, 29957, 29792, 29622, 29448, 29270, 29087,
    28899, 28708, 28512, 28311, 28107, 27898, 27685, 27468, 27246, 27021, 26791, 26558,
    26320, 26079, 25833, 25584, 25331, 25074, 24813, 24549, 24280, 24009, 23733, 23454,
    23172, 22886, 22596, 22303, 22007, 21707, 21404, 21098, 20789, 20477, 20161, 19843,
    19521, 19197, 18870, 18539, 18206, 17871, 17532, 17191, 16848, 16502, 16153, 15802,
    15448, 15093, 14735, 14374, 14012, 13647, 13281, 12912, 12542, 12169, 11795, 11419,
    11041, 10662, 10281, 9898, 9514, 9129, 8742, 8354, 7964, 7574, 7182, 6789,
    6395, 6000, 5604, 5208, 4810, 4412, 4014, 3614, 3214, 2814, 2413, 2012,
    1610, 1209, 807, 405, 3, -399, -801, -1203, -1605, -2006, -2407, -2808,
    -3209, -3609, -4008, -4407, -4805, -5202, -5599, -5995, -6390, -6783, -7176, -7568,
    -7959, -8348, -8736, -9123, -9509, -9893, -10276, -10657, -11036, -11414, -11790, -12164,
    -12537, -12907, -13276, -13642, -14007, -14369, -14730, -15088, -15444, -15797, -16148, -16497,
    -16843, -17187, -17528, -17866, -18202, -18535, -18865, -19192, -19517, -19838, -20157, -20473,
    -20785, -21094, -21400, -21703, -22003, -22299, -22592, -22882, -23168, -23450, -23729, -24005,
    -24277, -24545, -24810, -25071, -25328, -25581, -25830, -26076, -26317, -26555, -26788, -27018,
    -27243, -27465, -27682, -27895, -28104, -28309, -28509, -28705, -28897, -29084, -29267, -29446,
    -29620, -29790, -29955, -30116, -30272, -30424, -30571, -30713, -30851, -30984, -31113, -31237,
    -31356, -31470, -31580, -31685, -31785, -31880, -31971, -32057, -32138, -32214, -32285, -32351,
    -32413, -32469, -32521, -32568, -32610, -32647, -32679, -32706, -32728, -32746, -32758, -32765,
    -32768, -32766, -32758, -32746, -32729, -32707, -32680, -32648, -32611, -32569, -32522, -32471,
    -32414, -32353, -32286, -32215, -32139, -32058, -31973, -31882, -31787, -31687, -31582, -31473,
    -31358, -31239, -31116, -30987, -30854, -30716, -30574, -30427, -30276, -30119, -29959, -29794,
    -29624, -29450, -29271, -29088, -28901, -28709, -28514, -28313, -28109, -27900, -27687, -27470,
    -27248, -27023, -26794, -26560, -26323, -26081, -25836, -25586, -25333, -25076, -24816, -24551,
    -24283, -24011, -23736, -23457, -23174, -22888, -22599, -22306, -22010, -21710, -21407, -21101,
    -20792, -20480, -20164, -19846, -19524, -19200, -18873, -18542, -18210, -17874, -17535, -17194,
    -16851, -16505, -16156, -15805, -15452, -15096, -14738, -14378, -14015, -13651, -13284, -12916,
    -12545, -12173, -11798, -11422, -11045, -10665, -10284, -9902, -9518, -9132, -8745, -8357,
    -7968, -7577, -7185, -6792, -6398, -6004, -5608, -5211, -4814, -4416, -4017, -3618,
    -3218, -2817, -2417, -2015, -1614, -1212, -810, -408, 0,
};

static const uint16_t fixedAtanSmallSamples[129] = {
    0, 325, 651, 974, 1297, 1616, 1933, 2246, 2555, 2859, 3159, 3453,
    3742, 4024, 4301, 4572, 4836, 5093, 5344, 5588, 5826, 6057, 6282, 6500,
    6711, 6917, 7116, 7310, 7497, 7679, 7855, 8026, 8191, 8352, 8507, 8658,
    8804, 8946, 9083, 9217, 9346, 9471, 9593, 9710, 9825, 9936, 10044, 10149,
    10250, 10349, 10445, 10539, 10630, 10718, 10804, 10887, 10969, 11048, 11125, 11200,
    11273, 11344, 11414, 11481, 11547, 11612, 11675, 11736, 11796, 11854, 11911, 11967,
    12021, 12074, 12126, 12177, 12227, 12275, 12323, 12369, 12415, 12459, 12503, 12545,
    12587, 12628, 12668, 12707, 12746, 12783, 12820, 12857, 12892, 12927, 12961, 12995,
    13028, 13060, 13092, 13123, 13153, 13183, 13213, 13242, 13270, 13298, 13325, 13352,
    13379, 13405, 13431, 13456, 13481, 13505, 13529, 13553, 13576, 13599, 13621, 13644,
    13665, 13687, 13708, 13729, 13749, 13769, 13789, 13809, 13828,
};

static const uint16_t fixedAtanLargeSamples[129] = {
    0, 2555, 4836, 6711, 8191, 9346, 10250, 10969, 11547, 12021, 12415, 12746,
    13028, 13270, 13481, 13665, 13828, 13973, 14103, 14219, 14325, 14420, 14508, 14587,
    14661, 14729, 14791, 14849, 14903, 14954, 15001, 15045, 15086, 15125, 15162, 15197,
    15229, 15260, 15290, 15317, 15344, 15369, 15393, 15416, 15438, 15459, 15479, 15498,
    15516, 15534, 15551, 15567, 15583, 15598, 15612, 15626, 15640, 15653, 15665, 15677,
    15689, 15701, 15712, 15722, 15732, 15742, 15752, 15762, 15771, 15780, 15788, 15796,
    15805, 15813, 15820, 15828, 15835, 15842, 15849, 15856, 15862, 15869, 15875, 15881,
    15887, 15893, 15899, 15904, 15910, 15915, 15920, 15925, 15930, 15935, 15940, 15945,
    15949, 15954, 15958, 15962, 15967, 15971, 15975, 15979, 15983, 15986, 15990, 15994,
    15997, 16001, 16004, 16008, 16011, 16014, 16018, 16021, 16024, 16027, 16030, 16033,
    16036, 16039, 16042, 16044, 16047, 16050, 16052, 16055, 16058,
};

uint32_t vectorDefaultRandomState = 0x12345678u;

float rotationSineAngle(uint16_t angle)
{
    float radians = bScalarMultiply((float)angle, 0.0000958738019107841f);
    float sign = 1.0f;
    if (4.71238899230957f <= radians) {
        radians = bScalarAdd(radians, -6.2831854820251465f);
    } else if (1.5707963705062866f <= radians) {
        sign = -sign;
        radians = bScalarAdd(radians, -3.1415927410125732f);
    }

    const float squared = bScalarMultiply(radians, radians);
    const float cubed = bScalarMultiply(radians, squared);
    const float fifth = bScalarMultiply(cubed, squared);
    const float cubicTerm = bScalarMultiply(cubed, 0.1666666716337204f);
    const float seventh = bScalarMultiply(fifth, squared);
    float result = bScalarAdd(radians, -cubicTerm);
    const float fifthTerm = bScalarMultiply(fifth, 0.008333333767950535f);
    const float ninth = bScalarMultiply(seventh, squared);
    const float seventhTerm = bScalarMultiply(seventh, 0.00019841270113829523f);
    result = bScalarAdd(result, fifthTerm);
    const float ninthTerm = bScalarMultiply(ninth, 0.0000027557318844628753f);
    result = bScalarAdd(result, -seventhTerm);
    result = bScalarAdd(result, ninthTerm);
    return bScalarMultiply(result, sign);
}

float rotationSineRadians(float radians)
{
    const float angularCoordinate = bScalarMultiply(radians, 10430.3779296875f);
    const int32_t angle = convertFloatToInt32(angularCoordinate);
    return rotationSineAngle((uint16_t)angle);
}

int32_t vectorFixedReciprocal(int32_t value)
{
    if (!value) return INT32_MAX;
    return vectorMathSignedWord((uint32_t)(INT32_MAX / value) << 1);
}

int32_t vectorFixedDivide(int32_t numerator, int32_t denominator)
{
    if (!denominator) return !numerator ? 0 : numerator > 0 ? INT32_MAX : INT32_MIN;
    const int32_t reciprocal = INT32_MAX / denominator;
    return vectorMathSignedWord((uint32_t)vectorMathFixedProduct(numerator, reciprocal) << 1);
}

uint32_t vectorRandomSeed(uint32_t seed, uint32_t* state)
{
    *state = seed;
    return vectorRandomBounded(1, state);
}

uint32_t vectorRandomBounded(uint32_t bound, uint32_t* state)
{
    const uint32_t previous = *state;
    const uint32_t remainder = previous % bound;
    const uint32_t mixed = previous ^ 0x1d872b41u;
    const uint32_t shifted = mixed ^ (mixed >> 5);
    const uint32_t rotatedPart = mixed ^ (shifted << 27);
    *state = shifted ^ rotatedPart;
    return remainder;
}

float vectorRandomFloat(uint32_t* state, float maximum)
{
    const uint32_t value = vectorRandomBounded(INT32_MAX, state);
    // PAL25A43C/25A450: same bounded integer draw, then two scalar MULs.
    const float scale = bScalarMultiply(maximum, 4.6566128730773926e-10f);
    const float converted = value < 0x80000000u ? (float)value :
        (float)((value >> 1) | (value & 1)) * 2.0f;
    return bScalarMultiply(scale, converted);
}

int32_t vectorWrapInteger(int32_t value, int32_t period)
{
    const int32_t width = vectorMathSignedWord(vectorMathAbsoluteWord(period));
    const int32_t quotient = value / width;
    int32_t product = vectorMathSignedWord((uint32_t)((int64_t)quotient * width));
    if (value < 0) product = vectorMathSubtractWord(product, width);
    return vectorMathSubtractWord(value, product);
}

float vectorCosineAngle(uint16_t angle)
{ return rotationSineAngle((uint16_t)(angle + 16384u)); }

void vectorSineCosineAngle(float* sine, float* cosine, uint16_t angle)
{
    float radians = (float)angle * 0.0000958738019107841f;
    float sign = 1.0f;
    if (4.71238899230957f <= radians) radians = radians - 6.2831854820251465f;
    else if (1.5707963705062866f <= radians) { sign = -sign; radians = radians - 3.1415927410125732f; }
    const float squared = radians * radians;
    const float cubed = radians * squared;
    const float halfSquared = squared * 0.5f;
    const float fourth = squared * squared;
    const float fifth = cubed * squared;
    const float cubicTerm = cubed * 0.1666666716337204f;
    const float sixth = fourth * squared;
    const float seventh = fifth * squared;
    const float eighth = sixth * squared;
    const float fourthTerm = fourth * 0.0416666679084301f;
    const float ninth = seventh * squared;
    const float fifthTerm = fifth * 0.008333333767950535f;
    const float tenth = eighth * squared;
    const float eighthTerm = eighth * 0.000024801587642286904f;
    const float sixthTerm = sixth * 0.0013888889225199819f;
    const float seventhTerm = seventh * 0.00019841270113829523f;
    const float ninthTerm = ninth * 0.0000027557318844628753f;
    const float tenthTerm = tenth * 0.0000002755731998149713f;
    float sinValue = radians - cubicTerm;
    float cosValue = 1.0f - halfSquared;
    sinValue = sinValue + fifthTerm;
    cosValue = cosValue + fourthTerm;
    sinValue = sinValue - seventhTerm;
    cosValue = cosValue - sixthTerm;
    sinValue = sinValue + ninthTerm;
    cosValue = cosValue + eighthTerm;
    cosValue = cosValue - tenthTerm;
    *sine = sinValue * sign; *cosine = cosValue * sign;
}

uint16_t vectorArcSineAngle(float value)
{
    const int negative = value < 0.0f;
    if (negative) value = -value;
    if (1.0f <= value) return negative ? 49152u : 16384u;
    const int32_t coordinate = (int32_t)(value * 65536.0f);
    int32_t step = 32768, base = 32768, level = 0;
    if (coordinate >= step) {
        step >>= 1;
        for (;;) {
            base += step;
            ++level;
            if (coordinate < base || level >= 11) break;
            step >>= 1;
        }
    }
    base -= step;
    const int32_t segment = (coordinate - base) >> (11 - level);
    step >>= 4;
    const int32_t anchor = base + segment * step;
    const ArcSample* sample = &arcSamples[level * 16 + segment];
    const float anchorValue = (float)anchor * 0.0000152587890625f;
    const float delta = value - anchorValue;
    const float scaledDelta = delta * sample->slope;
    const int32_t interpolation = (int32_t)(scaledDelta * 65536.0f);
    const uint16_t result = (uint16_t)(sample->angle + interpolation);
    return negative ? (uint16_t)(0u - result) : result;
}

int32_t vectorFixedSineAngle(uint16_t angle)
{
    const uint32_t index = angle >> 7;
    const int32_t first = (int32_t)fixedSineSamples[index] * 2;
    const int32_t second = (int32_t)fixedSineSamples[index + 1] * 2;
    const int32_t fraction = (uint16_t)(angle << 9);
    const int32_t product = vectorMathSignedWord((uint32_t)((int64_t)(second - first) * fraction));
    return vectorMathAddWord(first, vectorMathShiftRight(product, 16));
}

void vectorFixedSineCosineAngle(int32_t* sine, int32_t* cosine, uint16_t angle)
{
    *sine = vectorFixedSineAngle(angle);
    *cosine = vectorFixedSineAngle((uint16_t)(angle + 16384u));
}

uint16_t vectorFixedAtanAngle(int32_t value)
{
    const int negative = value < 0;
    if (negative) value = vectorMathSignedWord(0u - (uint32_t)value);
    int32_t first, second, fraction;
    if (value <= ((1 << 21) - 1)) {
        const uint16_t* samples;
        uint32_t index;
        if (value <= 0x0003ffff) {
            index = (uint32_t)vectorMathShiftRight(value, 11); samples = fixedAtanSmallSamples;
            fraction = (uint16_t)((uint32_t)value << 5);
        } else {
            index = (uint32_t)vectorMathShiftRight(value, 14); samples = fixedAtanLargeSamples;
            fraction = (uint16_t)((uint32_t)value << 2);
        }
        first = samples[index]; second = samples[index + 1];
    } else if (value <= 0x00ffffff) {
        first = 0x3eba; second = 0x3fd7; fraction = vectorMathShiftRight(value, 8);
    } else { first = second = 0x3fff; fraction = 0; }
    const int32_t product = vectorMathSignedWord((uint32_t)((int64_t)(second - first) * fraction));
    const uint16_t result = (uint16_t)vectorMathAddWord(first, vectorMathShiftRight(product, 16));
    return negative ? (uint16_t)(0u - result) : result;
}

uint16_t vectorFixedAtan2Angle(int32_t first, int32_t second)
{
    int quadrant = first < 0;
    if (first < 0) first = vectorMathSignedWord(0u - (uint32_t)first);
    if (second < 0) { quadrant ^= 3; second = vectorMathSignedWord(0u - (uint32_t)second); }
    const uint16_t angle = first <= vectorMathShiftRight(second, 14) ? 16384u :
        vectorFixedAtanAngle(vectorFixedDivide(second, first));
    if (!quadrant) return angle;
    if (quadrant == 1) return (uint16_t)(32768u - angle);
    if (quadrant == 2) return (uint16_t)(angle - 32768u);
    return (uint16_t)(0u - angle);
}

VectorPolar2* vectorPolarFromCartesian2(VectorPolar2* output, const float input[2])
{
    const float x = input[0], y = input[1];
    const float xx = x * x, yy = y * y;
    const float radius = sqrtf(xx + yy);
    output->angle = vectorAtan2Angle(x, y); output->radius = radius;
    return output;
}

uint32_t vectorDefaultRandomBounded(uint32_t bound)
{
    return vectorRandomBounded(bound, &vectorDefaultRandomState);
}

float vectorDefaultRandomFloat(float maximum)
{
    return vectorRandomFloat(&vectorDefaultRandomState, maximum);
}

uint16_t vectorAtan2Angle(float first, float second)
{
    int quadrant = first < 0.0f;
    if (second < 0.0f) quadrant ^= 3;
    uint16_t angle;
    if (fabsf(second) < fabsf(first)) {
        const float firstSquared = first * first, secondSquared = second * second;
        const float squared = secondSquared + firstSquared;
        const float ratio = 0.00009999999747378752f < squared && second != 0.0f ?
            finiteRatio(fabsf(second), sqrtf(squared)) : 0.0f;
        angle = vectorArcSineAngle(ratio);
    } else {
        const float secondSquared = second * second, firstSquared = first * first;
        const float squared = secondSquared + firstSquared;
        const float ratio = 0.00009999999747378752f < squared && first != 0.0f ?
            finiteRatio(fabsf(first), sqrtf(squared)) : 0.0f;
        angle = (uint16_t)(16384u - vectorArcSineAngle(ratio));
    }
    if (!quadrant) return angle;
    if (quadrant == 3) return (uint16_t)(0u - angle);
    if (quadrant == 1) return (uint16_t)(32768u - angle);
    return (uint16_t)(angle - 32768u);
}
