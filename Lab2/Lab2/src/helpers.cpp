#include <app.hpp>

void utoa10(uint32_t value, char *out)
{
    char tmp[11];
    int i = 0;

    if (value == 0)
    {
        out[0] = '0';
        out[1] = '\0';
        return;
    }

    while (value > 0)
    {
        tmp[i++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    int j = 0;
    while (i > 0)
    {
        out[j++] = tmp[--i];
    }
    out[j] = '\0';
}

bool parsePercent(const char *s, uint8_t &out)
{
    if (*s == '\0')
        return false;

    uint32_t value = 0;
    const char *p = s;
    while (*p >= '0' && *p <= '9')
    {
        value = value * 10U + (uint32_t)(*p - '0');
        if (value > 100U)
            return false;
        ++p;
    }

    if (p == s || *p != '\0')
        return false;

    out = (uint8_t)value;
    return true;
}