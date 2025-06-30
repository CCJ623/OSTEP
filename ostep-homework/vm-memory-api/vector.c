#include <stdlib.h>
#include <error.h>
#include <stdio.h>

struct Vector
{
    int* begin_;
    int* end_;
    int* capacity_end_;
};

void initVector(struct Vector* v)
{
    v->begin_ = malloc(sizeof(int));
    v->end_ = v->begin_;
    v->capacity_end_ = v->begin_ + 1;
}

void freeVector(struct Vector* v)
{
    free(v->begin_);
    v->begin_ = v->end_ = v->capacity_end_ = NULL;
}

size_t getVectorSize(struct Vector* v)
{
    return v->end_ - v->begin_;
}

void expandVectorSpace(struct Vector* v)
{
    size_t new_size = (v->end_ - v->begin_) * 2;
    v->begin_ = realloc(v->begin_, new_size * sizeof(int));
    if (v->begin_ == NULL)
    {
        perror("can NOT allocate memory!");
        exit(-1);
    }
    v->end_ = v->begin_ + (new_size / 2);
    v->capacity_end_ = v->begin_ + new_size;
}

void pushBack(struct Vector* v, int value)
{
    if (v->end_ != v->capacity_end_)
    {
        *(v->end_) = value;
        ++(v->end_);
    }
    else
    {
        expandVectorSpace(v);
        *(v->end_) = value;
        ++(v->end_);
    }
}

int getVectorValue(struct Vector* v, size_t index)
{
    if (index >= getVectorSize(v))
    {
        perror("out of range");
        exit(-1);
    }

    return (v->begin_)[index];
}

void setVectorValue(struct  Vector* v, size_t index, int value)
{
    if (index >= getVectorSize(v))
    {
        perror("out of range");
        exit(-1);
    }

    (v->begin_)[index] = value;
};


static const size_t LOOP_TIME = 1e6;

int main()
{
    struct Vector v;
    initVector(&v);
    for (size_t i = 0; i != LOOP_TIME; ++i)
    {
        pushBack(&v, i);
        int value = getVectorValue(&v, i);
        setVectorValue(&v, i, value * 2);
        //printf("%d ", getVectorValue(&v, i));
    }
    freeVector(&v);
}