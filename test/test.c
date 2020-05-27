#define EXIT *(volatile unsigned int *)(0xffffffff)
#define PRINT *(volatile unsigned int *)(0xfffffffe)
#define DEBUG *(volatile unsigned int *)(0xfffffff0)

void *memcpy(void *dest, const void *src, int len)
{
    char *tmp = dest;
    const char *s = src;

    while (len--)
        *tmp++ = *s++;
    return dest;
}

int strlen(const char *s)
{
    const char *sc;

    for (sc = s; *sc != '\0'; ++sc)
        ;
    return sc - s;
}

void insertion_sort(int a[], int n)
{
    for (int i = 1, j; i < n; i++)
    {
        int x = a[i];
        for (j = i; j > 0 && a[j - 1] > x; j--)
        {
            a[j] = a[j - 1];
        }
        a[j] = x;
    }
}

static int strcmp(const char * s1, const char * s2)
{
	int res;

	while (1)
	{
		if ((res = *s1 - *s2++) != 0 || !*s1++)
			break;
	}
	return res;
}

static int strncmp(const char * s1, const char * s2, int n)
{
	int __res = 0;

	while (n)
	{
		if ((__res = *s1 - *s2++) != 0 || !*s1++)
			break;
		n--;
	}
	return __res;
}

void print(char c)
{
    PRINT = c;
}

void prints(char buf[])
{
    int len = strlen(buf);
    for (int i = 0; i < len; i++)
        print(buf[i]);
}

#define ARRAY_SIZE(x) ((sizeof(x)) / (sizeof(x[0])))

void printi(int i)
{
    int a = i % 10;
    int b = i / 10;
    if (b)
        printi(b);
    print('0' + a);
}
void test_sort()
{
    int a[] = {3, 87, 25, 0, 32, 82, 4, 49, 17, 84, 43, 19, 77, 20, 51, 69, 85, 16, 44, 74};
    for (int i = 0; i < ARRAY_SIZE(a); i++)
        printi(a[i]), prints(" ");
    prints("\n");
    insertion_sort(a, ARRAY_SIZE(a));
    for (int i = 0; i < ARRAY_SIZE(a); i++)
        printi(a[i]), prints(" ");
    prints("\n");
}

#define expect(a, b) ((a) == (b) ? (prints("ok  " #a " == " #b "\n")) : (prints("err  " #a " == " #b "\n")))
#define expect_string(a, b) ((!strcmp((a),(b))) ? (prints("ok  " #a " == " #b "\n")) : (prints("err  " #a " == " #b "\n")))


static int I(int i){
    return i;
}

static long L(long i){
    return i;
}

static long long LL(long long i){
    return i;
}

static float F(float i){
    return i;
}

static double D(double i){
    return i;
}

static const char * S(const char  *i){
    return i;
}

static void test_basic() {
    expect(0, I(0));
    expect(3, I(1) + I(2));
    expect(3, I(1) + I(2));
    expect(10, I(1) + I(2) + I(3) + I(4));
    expect(11, I(1) + I(2) * I(3) + I(4));
    expect(14, I(1) * I(2) + I(3) * I(4));
    expect(4, 4 / I(2) + 6 / I(3));
    expect(4, I(24) / I(2) / I(3));
    expect(3, I(24) % 7);
    expect(0, I(24) % I(3));
    expect(98, 'a' + I(1));
    int a = 0 - I(1);
    expect(0 - I(1), a);
    expect(-I(1), a);
    expect(0, a + I(1));
    expect(1, +I(1));
    expect(1, (unsigned)4000000001 % I(2));
}

static void test_relative() {
    expect(1, I(1) > 0);
    expect(1, I(0) < I(1));
    expect(0, I(1) < I(0));
    expect(0, I(0) > I(1));
    expect(0, I(1) > I(1));
    expect(0, I(1) < I(1));
    expect(1, I(1) >= I(0));
    expect(1, I(0) <= I(1));
    expect(0, I(1) <= I(0));
    expect(0, I(0) >= I(1));
    expect(1, I(1) >= I(1));
    expect(1, I(1) <= I(1));
    expect(1, 0xFFFFFFFFU > I(1));
    expect(1, 1 < 0xFFFFFFFFU);
    expect(1, 0xFFFFFFFFU >= I(1));
    expect(1, 1 <= 0xFFFFFFFFU);
    expect(1, -I(1) > 1U);
    expect(1, -I(1) >= 1U);

    expect(1, -L(1L) > 1U);
    expect(1, -L(1L) >= 1U);

    expect(0, -LL(1LL) > 1U);
    expect(0, -LL(1LL) >= 1U);

    expect(0, D(1.0) < 0.0);
    expect(1, D(0.0) < 1.0);
}

static void test_inc_dec() {
    int a = 15;
    expect(15, a++);
    expect(16, a);
    expect(16, a--);
    expect(15, a);
    expect(14, --a);
    expect(14, a);
    expect(15, ++a);
    expect(15, a);
}

static void test_bool() {
    expect(0, !I(1));
    expect(1 ,!I(0));
}

static void test_unary()
{
    char x = I(2);
    short y = I(2);
    int z = 2;
    expect(-2, -x);
    expect(-2, -y);
    expect(-2, -z);
}

static void test_or()
{
    expect(3, I(1) | I(2));
    expect(7, I(2) | I(5));
    expect(7, I(2) | I(7));
}

static void test_and()
{
    expect(0, I(1) & 2);
    expect(2, I(2) & 7);
}

static void test_not()
{
    expect(-1, ~I(0));
    expect(-3, ~I(2));
    expect(0, ~-I(1));
}

static void test_xor()
{
    expect(10, I(15) ^ I(5));
}

static void test_shift()
{
    expect(16, I(1) << I(4) );
    expect(48, I(3) << I(4));

    expect(1, 15 >> I(3) );
    expect(2, 8 >> I(2));

    expect(1, ((unsigned)-1) >> I(31));
}

void test_bit()
{
    prints("bitwise operators\n");
    test_or();
    test_and();
    test_not();
    test_xor();
    test_shift();
}

float tf1(float a)  { return a; }
float tf2(double a) { return a; }
float tf3(int a)    { return a; }

double td1(float a)  { return a; }
double td2(double a) { return a; }

double recursive(double a) {
    if (a < 10) return a;
    return recursive(3.33);
}

void test_float() {
    prints("float\n");

    expect(0.7, D(.7));
    float v1 = D(10.0);
    float v2 = v1;
    expect(10.0, v1);
    expect(10.0, v2);

    double v3 = D(20.0);
    double v4 = v3;
    expect(20.0, v3);
    expect(20.0, v4);

    expect(1.0, D(1.0));
    expect(1.5, D(1.0) + 0.5);
    expect(0.5, D(1.0) - 0.5);
    expect(2.0, D(1.0) * 2.0);
    expect(0.25, D(1.0) / 4.0);

    expect(3.0, D(1.0) + 2);
    expect(2.5, D(5) - 2.5);
    expect(2.0, D(1.0) * 2);
    expect(0.25, D(1.0) / 4);

    expect(10.5, tf1(10.5));
    expect(10.0, tf1(10));
    expect(0, 10.6 == tf2(10.6));
    expect(10.0, tf2(10));
    expect(10.0, tf3(10.7));
    expect(10.0, tf3(10));

    expect(1.0, tf1(1.0));
    expect(10.0, tf1(10));
    expect(2.0, tf2(2.0));
    expect(10.0, tf2(10));
    expect(11.0, tf3(11.5));
    expect(10.0, tf3(10));

    expect(3.33, recursive(100));
}

static void verify(int *expected, int *got, int len) {
    for (int i = 0; i < len; i++)
        expect(expected[i], got[i]);
}

static void verify_short(short *expected, short *got, int len) {
    for (int i = 0; i < len; i++)
        expect(expected[i], got[i]);
}

static void test_array() {
    int x[] = { I(1), 3, 5 };
    expect(1, x[0]);
    expect(3, x[1]);
    expect(5, x[2]);

    int ye[] = { I(1), 3, 5, 2, 4, 6, 3, 5, 7, 0, 0, 0 };
    int y1[4][3] = { { I(1), 3, 5 }, { 2, 4, 6 }, { 3, 5, 7 }, };
    verify(ye, y1, 12);
    int y2[4][3] = { 1, 3, 5, 2, 4, 6, 3, 5, 7 };
    verify(ye, y2, 12);

    int ze[] = { I(1), 0, 0, 2, 0, 0, 3, 0, 0, 4, 0, 0 };
    int z[4][3] = { { I(1) }, { 2 }, { 3 }, { 4 } };
    verify(ze, z, 12);

    short qe[24] = { I(1), 0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 4, 5, 6 };
    short q[4][3][2] = { { I(1) }, { 2, 3 }, { 4, 5, 6 } };
    verify_short(qe, q, 24);

    int a[] = {{{ 3 }}};
    expect(3, a[0]);
}

static void test_string() {
    char s[] = "abc";
    expect_string(S("abc"), s);
    char t[] = { S("def") };
    expect_string(S("def"), t);
}

static void test_struct() {
    int we[] = { 1, 0, 0, 0, 2, 0, 0, 0 };
    struct { int a[3]; int b; } w[] = { { 1 }, 2 };
    verify(we, &w, 8);
}

static void test_primitive() {
    int a = { 59 };
    expect(59, a);
}

static void test_nested() {
    struct {
        struct {
            struct { int a; int b; } x;
            struct { char c[8]; } y;
        } w;
    } v = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, };
    expect(1, v.w.x.a);
    expect(2, v.w.x.b);
    expect(3, v.w.y.c[0]);
    expect(10, v.w.y.c[7]);
}

static void test_array_designator() {
    int v[3] = { [1] = 5 };
    expect(0, v[0]);
    expect(5, v[1]);
    expect(0, v[2]);

    struct { int a, b; } x[2] = { [1] = { 1, 2 } };
    expect(0, x[0].a);
    expect(0, x[0].b);
    expect(1, x[1].a);
    expect(2, x[1].b);

    struct { int a, b; } x2[3] = { [1] = 1, 2, 3, 4 };
    expect(0, x2[0].a);
    expect(0, x2[0].b);
    expect(1, x2[1].a);
    expect(2, x2[1].b);
    expect(3, x2[2].a);
    expect(4, x2[2].b);

    int x3[] = { [2] = 3, [0] = 1, 2 };
    expect(1, x3[0]);
    expect(2, x3[1]);
    expect(3, x3[2]);
}

static void test_struct_designator() {
    struct { int x; int y; } v1 = { .y = 1, .x = 5 };
    expect(5, v1.x);
    expect(1, v1.y);

    struct { int x; int y; } v2 = { .y = 7 };
    expect(7, v2.y);

    struct { int x; int y; int z; } v3 = { .y = 12, 17 };
    expect(12, v3.y);
    expect(17, v3.z);
}

static void test_complex_designator() {
    struct { struct { int a, b; } x[3]; } y[] = {
        [1].x[0].b = 5, 6, 7, 8, 9,
        [0].x[2].b = 10, 11
    };
    expect(0, y[0].x[0].a);
    expect(0, y[0].x[0].b);
    expect(0, y[0].x[1].a);
    expect(0, y[0].x[1].b);
    expect(0, y[0].x[2].a);
    expect(10, y[0].x[2].b);
    expect(11, y[1].x[0].a);
    expect(5, y[1].x[0].b);
    expect(6, y[1].x[1].a);
    expect(7, y[1].x[1].b);
    expect(8, y[1].x[2].a);
    expect(9, y[1].x[2].b);

    int y2[][3] = { [0][0] = 1, [1][0] = 3 };
    expect(1, y2[0][0]);
    expect(3, y2[1][0]);

    struct { int a, b[3]; } y3 = { .a = 1, .b[0] = 10, .b[1] = 11 };
    expect(1, y3.a);
    expect(10, y3.b[0]);
    expect(11, y3.b[1]);
    expect(0, y3.b[2]);
}

static void test_zero() {
    struct tag { int x, y; };
    struct tag v0 = (struct tag){ I(6) };
    expect(6, v0.x);
    expect(0, v0.y);

    struct { int x; int y; } v1 = { I(6) };
    expect(6, v1.x);
    expect(0, v1.y);

    struct { int x; int y; } v2 = { .y = I(3) };
    expect(0, v2.x);
    expect(3, v2.y);

    struct { union { int x, y; }; } v3 = { .x = 61 };
    expect(61, v3.x);
}


static void test_typedef() {
    typedef int A[];
    A a = { 1, 2 };
    A b = { 3, 4, 5 };
    expect(2, sizeof(a) / sizeof(*a));
    expect(3, sizeof(b) / sizeof(*b));
}

static void test_excessive() {
    char x1[3] = { 1, 2, 3, 4, 5 };
    expect(3, sizeof(x1));

    char x2[3] = "abcdefg";
    expect(3, sizeof(x2));
    expect(0, strncmp(S("abc"), x2, 3));
}

void test_init() {
    prints("initializer");

    test_array();
    test_string();
    test_struct();
    test_primitive();
    test_nested();
    test_array_designator();
    test_struct_designator();
    test_complex_designator();
    test_zero();
    test_typedef();
    test_excessive();
}

void test()
{
    test_sort();
    test_basic();
    test_bool();
    test_inc_dec();
    test_unary();
    test_relative();
    test_bit();
    test_float();
    test_init();
}
void exit()
{
    prints("call exit\n");
    EXIT = 0;
}