/*! @file Philox432.c.h
*  @brief Code for the Philox432 generator common to the host and device
*/

#pragma once
#ifndef PRIVATE_PHILOX432_CH
#define PRIVATE_PHILOX432_CH

#include "Random123/philox.h" 

#define Philox432_NORM_double    1.0 / 0x100000000L   // 1.0 /2^32 
#define Philox432_NORM_float     2.32830644e-010; 

hcrngPhilox432Counter hcrngPhilox432Add(hcrngPhilox432Counter a, hcrngPhilox432Counter b) restrict (amp, cpu)
{
	hcrngPhilox432Counter c;

	c.L.lsb = a.L.lsb + b.L.lsb;
	c.L.msb = a.L.msb + b.L.msb + (c.L.lsb < a.L.lsb);

	c.H.lsb = a.H.lsb + b.H.lsb + (c.L.msb < a.L.msb);
	c.H.msb = a.H.msb + b.H.msb + (c.H.lsb < a.H.lsb);

	return c;
}

hcrngPhilox432Counter hcrngPhilox432Substract(hcrngPhilox432Counter a, hcrngPhilox432Counter b) restrict (amp, cpu) 
{
	hcrngPhilox432Counter c;

	c.L.lsb = a.L.lsb - b.L.lsb;
	c.L.msb = a.L.msb - b.L.msb - (c.L.lsb > a.L.lsb);

	c.H.lsb = a.H.lsb - b.H.lsb - (c.L.msb > a.L.msb);
	c.H.msb = a.H.msb - b.H.msb - (c.H.lsb > a.H.lsb);

	return c;
}

hcrngStatus hcrngPhilox432CopyOverStreams(size_t count, hcrngPhilox432Stream* destStreams, const hcrngPhilox432Stream* srcStreams) restrict (amp, cpu)
{
	for (size_t i = 0; i < count; i++)
		destStreams[i] = srcStreams[i];

	return HCRNG_SUCCESS;
}

void hcrngPhilox432GenerateDeck(hcrngPhilox432StreamState *currentState) restrict (amp, cpu) 
{
	//Default key
	philox4x32_key_t k = { { 0, 0 } };

	//get the currect state
	philox4x32_ctr_t c = { { 0 } };
	c.v[0] = currentState->ctr.L.lsb;
	c.v[1] = currentState->ctr.L.msb;
	c.v[2] = currentState->ctr.H.lsb;
	c.v[3] = currentState->ctr.H.msb;

	//Generate 4 uint and store them into the stream state
	philox4x32_ctr_t r = philox4x32(c, k);
	currentState->deck[3] = r.v[0];
	currentState->deck[2] = r.v[1];
	currentState->deck[1] = r.v[2];
	currentState->deck[0] = r.v[3];
}

/*! @brief Advance the rng one step
*/
static unsigned int hcrngPhilox432NextState(hcrngPhilox432StreamState *currentState) restrict (amp, cpu) {

	if (currentState->deckIndex == 0)
	{
		hcrngPhilox432GenerateDeck(currentState);
	
	}

	unsigned int result = currentState->deck[currentState->deckIndex];
	
	currentState->deckIndex++;

	// Advance to the next Counter.
	if (currentState->deckIndex == 4) {

		hcrngPhilox432Counter incBy1 = { { 0, 0 }, { 0, 1 } };
		currentState->ctr = hcrngPhilox432Add(currentState->ctr, incBy1);

		currentState->deckIndex = 0;
		hcrngPhilox432GenerateDeck(currentState);
	}

	return result;

}
// The following would be much hceaner with C++ templates instead of macros.

// We use an underscore on the r.h.s. to avoid potential recursion with certain
// preprocessors.
#define IMPLEMENT_GENERATE_FOR_TYPE(fptype) \
	\
	fptype hcrngPhilox432RandomU01_##fptype(hcrngPhilox432Stream* stream) restrict (amp, cpu) { \
	    return (hcrngPhilox432NextState(&stream->current) + 0.5) * Philox432_NORM_##fptype; \
	} \
	\
	int hcrngPhilox432RandomInteger_##fptype(hcrngPhilox432Stream* stream, int i, int j) restrict (amp, cpu) { \
	    return i + (int)((j - i + 1) * hcrngPhilox432RandomU01_##fptype(stream)); \
	} \
	\
	hcrngStatus hcrngPhilox432RandomU01Array_##fptype(hcrngPhilox432Stream* stream, size_t count, fptype* buffer) restrict (amp, cpu) { \
		for (size_t i = 0; i < count; i++)  \
			buffer[i] = hcrngPhilox432RandomU01_##fptype(stream); \
		return HCRNG_SUCCESS; \
	} \
	\
	hcrngStatus hcrngPhilox432RandomIntegerArray_##fptype(hcrngPhilox432Stream* stream, int i, int j, size_t count, int* buffer) restrict (amp, cpu) { \
		for (size_t k = 0; k < count; k++) \
			buffer[k] = hcrngPhilox432RandomInteger_##fptype(stream, i, j); \
		return HCRNG_SUCCESS; \
	}

// On the host, implement everything.
// On the device, implement only what is required to avoid hcuttering memory.
#if defined(HCRNG_SINGLE_PRECISION)  
IMPLEMENT_GENERATE_FOR_TYPE(float)
#endif
#if !defined(HCRNG_SINGLE_PRECISION) 
IMPLEMENT_GENERATE_FOR_TYPE(double)
#endif

// Clean up macros, especially to avoid polluting device code.
#undef IMPLEMENT_GENERATE_FOR_TYPE



hcrngStatus hcrngPhilox432RewindStreams(size_t count, hcrngPhilox432Stream* streams) restrict (amp, cpu)
{
	//Reset current state to the stream initial state
	for (size_t j = 0; j < count; j++) {
		streams[j].current = streams[j].substream = streams[j].initial;
}

	return HCRNG_SUCCESS;
}


hcrngStatus hcrngPhilox432RewindSubstreams(size_t count, hcrngPhilox432Stream* streams) restrict (amp, cpu)
{
	//Reset current state to the subStream initial state
	for (size_t j = 0; j < count; j++) {
		streams[j].current = streams[j].substream;
	}

	return HCRNG_SUCCESS;
}

void Philox432ResetNextSubStream(hcrngPhilox432Stream* stream)restrict (amp, cpu) {

	//2^64 states
	hcrngPhilox432Counter steps = { { 0, 1 }, { 0, 0 } };

	//move the substream counter 2^64 steps forward.
	stream->substream.ctr = hcrngPhilox432Add(stream->substream.ctr, steps);

	hcrngPhilox432RewindSubstreams(1, stream);
}

hcrngStatus hcrngPhilox432ForwardToNextSubstreams(size_t count, hcrngPhilox432Stream* streams) restrict (amp, cpu) 
{
	for (size_t k = 0; k < count; k++) {

		Philox432ResetNextSubStream(&streams[k]);
	}

	return HCRNG_SUCCESS;
}

hcrngStatus hcrngPhilox432MakeOverSubstreams(hcrngPhilox432Stream* stream, size_t count, hcrngPhilox432Stream* substreams) restrict (amp, cpu)
{
	for (size_t i = 0; i < count; i++) {
		hcrngStatus err;
		// snapshot current stream into substreams[i]
		err = hcrngPhilox432CopyOverStreams(1, &substreams[i], stream);
		if (err != HCRNG_SUCCESS)
			return err;
		// advance to next substream
		err = hcrngPhilox432ForwardToNextSubstreams(1, stream);
		if (err != HCRNG_SUCCESS)
			return err;
	}
	return HCRNG_SUCCESS;
}

hcrngStatus hcrngPhilox432CopyOverStreamsFromGlobal(size_t count, hcrngPhilox432Stream* destStreams, hcrngPhilox432Stream* srcStreams) restrict (amp)
{
        for (size_t i = 0; i < count; i++) {
                destStreams[i].current = srcStreams[i].current;
                destStreams[i].initial = srcStreams[i].initial;
#ifdef HCRNG_ENABLE_SUBSTREAMS
                destStreams[i].substream = srcStreams[i].substream;
#endif
        }

        return HCRNG_SUCCESS;
}

hcrngStatus hcrngPhilox432CopyOverStreamsToGlobal(size_t count, hcrngPhilox432Stream* destStreams, hcrngPhilox432Stream* srcStreams) restrict (amp)
{
        for (size_t i = 0; i < count; i++) {
                destStreams[i].current = srcStreams[i].current;
                destStreams[i].initial = srcStreams[i].initial;
#ifdef HCRNG_ENABLE_SUBSTREAMS
                destStreams[i].substream = srcStreams[i].substream;
#endif
        }

        return HCRNG_SUCCESS;
}


#endif // PRIVATE_Philox432_CH
