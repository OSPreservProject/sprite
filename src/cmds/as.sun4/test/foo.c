
static foo()
{
	return 0x12345678;
}

static bar()
{
	return 0x123;
}

static baz()
{
	return 0xfffffff0;
}

int x;

static int * bum()
{
	return &x;
}

main()
{
    printf("foo = %x, bar = %x, baz = %x, bum = %x, &x = %x\n",
	foo(), bar(), baz(), bum(), &x);
}
