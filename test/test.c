#define EXIT *(volatile unsigned int *)(0xffffffff)
#define PRINT *(volatile unsigned int *)(0xfffffffe)

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

void printi(int i){
    int a = i%10;
    int b = i/10;
    if(b)
        printi(b);
    print('0'+a);

}
void test1()
{
    int a[] = {3,87,25,0,32,82,4,49,17,84,43,19,77,20,51,69,85,16,44,74};
    for (int i = 0; i < ARRAY_SIZE(a); i++)
        printi(a[i]),prints(" ");
    prints("\n");
    insertion_sort(a, ARRAY_SIZE(a));
    for (int i = 0; i < ARRAY_SIZE(a); i++)
        printi(a[i]),prints(" ");
    prints("\n");
}

#define expect(a,b) ((a) == (b)?(prints("ok  " #a" == "#b "\n")):(prints("error\n")))
void test2()
{
    int a=131, b=45;
    expect(a+b,131+45);
    expect(a-b,131-45);
    expect(a*b,131*45);
    expect(a/b,131/45);
    expect(a%b,131%45);
    expect(a&b,131&45);
    expect(a|b,131|45);
    expect(a^b,131^45);
}

void test()
{
    test1();
    test2();
}
void exit()
{
    prints("call exit\n");
    EXIT = 0;
}