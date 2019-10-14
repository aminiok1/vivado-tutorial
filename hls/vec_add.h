#include <ap_axi_sdata.h>


/*
 template<int D,int U,int TI,int TD>
  struct ap_axiu{
    ap_uint<D>       data;
    ap_uint<U>       user;
    ap_uint<TI>      id;
    ap_uint<TD>      dest;
  };

  ap_axiu struct sets the width of side band signals
  The only important parameter here is data width which we set to 64 (double precision floating point)
 */
typedef ap_axiu<64, 4, 5, 5> AXI_VAL;

// vector input size
#define DIM 128

// function prototype
void accelerator_kernel(AXI_VAL in_stream[DIM + DIM], AXI_VAL out_stream[DIM]);

// Push-Pop Stream functions of AXI-Stream
// we will use the following two functions to retrieve the input from axi interface 
// and also push the results to the axi interface
template<typename T, int U, int TI, int TD>
T pop_stream(ap_axiu <sizeof(T) * 8, U, TI, TD> const &e)
{
#pragma HLS INLINE

	//assert(sizeof(T) == sizeof(double));
	union
	{
		long long ival;
		T oval;
	} converter;
	converter.ival = e.data;
	T ret = converter.oval;

	// axi signals
	volatile ap_uint<sizeof(T)> strb = e.strb;
	volatile ap_uint<sizeof(T)> keep = e.keep;
	volatile ap_uint<U> user = e.user;
	volatile ap_uint<1> last = e.last;
	volatile ap_uint<TI> id = e.id;
	volatile ap_uint<TD> dest = e.dest;

	return ret;
}

template <typename T, int U, int TI, int TD>
ap_axiu <sizeof(T) * 8, U, TI, TD> push_stream(T const &v, bool last = false)
{
#pragma HLS INLINE
	ap_axiu<sizeof(T) * 8, U, TI, TD> e;

	//assert(sizeof(T) == sizeof(double));
	union
	{
		long long oval;
		T ival;
	} converter;
	converter.ival = v;
	e.data = converter.oval;

	// setting axi signals
	e.strb = 0xFF;
	e.keep = 0xFF; //e.strb;
	e.user = 0;
	e.last = last ? 1 : 0;
	e.id = 0;
	e.dest = 0;
	return e;
}
