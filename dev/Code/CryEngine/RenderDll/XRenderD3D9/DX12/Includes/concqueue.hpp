#ifndef __CONC_QUEUE_INCLUDED__
#define __CONC_QUEUE_INCLUDED__

#include "concqueue-mpmc-bounded.hpp"
#include "concqueue-mpsc.hpp"
#include "concqueue-spsc.hpp"
#include "concqueue-spsc-bounded.hpp"

#define BoundMPMC   concqueue::mpmc_bounded_queue_t
#define BoundSPSC   concqueue::spsc_bounded_queue_t
#define UnboundMPSC concqueue::mpsc_queue_t
#define UnboundSPSC concqueue::spsc_queue_t

template<template <typename> class queue, typename subject>
class ConcQueue
    : public queue<subject>
{
};

#endif
