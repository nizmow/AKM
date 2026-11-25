#include <stddef.h>

void shelsort(void *base, size_t count, size_t size,
              int (*compare)(const void *, const void *))
{
    char *bytes = base;

    if (!bytes || !compare || size == 0)
        return;

    for (size_t gap = count / 2; gap != 0; gap >>= 1)
    {
        for (size_t i = gap; i < count; ++i)
        {
            size_t j = i;

            while (j >= gap)
            {
                char *left = bytes + (j - gap) * size;
                char *right = bytes + j * size;
                if (compare(left, right) <= 0)
                    break;

                for (size_t k = 0; k < size; ++k)
                {
                    char tmp = left[k];
                    left[k] = right[k];
                    right[k] = tmp;
                }
                j -= gap;
            }
        }
    }
}
