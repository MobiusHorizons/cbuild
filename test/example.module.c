export {
#include <stdbool.h>
}

export struct a {
	bool b;
	struct {
		char a;
	} c;
};

export enum test{
	a = 0,
	b,
	c,
	d,
	e,
	f,
	g,
} as test2;

export typedef struct a {
	bool b;
	struct {
		char a;
	} c;
} c as b;

export
union a {
	float a;
	long  b;
	int   c;
};

export typedef void * (*callback)(void);

export const long long int * test_fn(void) {
	// body
}
