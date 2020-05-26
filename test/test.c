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
}
void exit()
{
    prints("call exit\n");
    EXIT = 0;
}