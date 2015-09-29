char	*
gcvt(value, digits, buff)
double	value;
int	digits;
char	*buff;
{
	sprintf(buff, "%*g", digits, value);
	return (buff);
}
