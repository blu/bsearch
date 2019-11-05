#ifndef __rand_H
#define __rand_H

namespace rnd {

int rand_r(unsigned int *seed);

const unsigned rand_max = (1LL << 32) - 1;

} // namespace rnd

#endif // __rand_H
