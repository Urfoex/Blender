/*
 *	lame utility library source file
 *
 *	Copyright (c) 1999 Albert L Faber
 *	Copyright (c) 2000-2005 Alexander Leidinger
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: util.c,v 1.117.2.1 2005/11/20 14:08:25 bouvigne Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define PRECOMPUTE

#include "util.h"
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#if defined(__FreeBSD__) && !defined(__alpha__)
# include <machine/floatingpoint.h>
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/***********************************************************************
*
*  Global Function Definitions
*
***********************************************************************/
/*empty and close mallocs in gfc */

void  freegfc ( lame_internal_flags* const gfc )   /* bit stream structure */
{
    int  i;


    for ( i = 0 ; i <= 2*BPC; i++ )
        if ( gfc->blackfilt[i] != NULL ) {
            free ( gfc->blackfilt[i] );
	    gfc->blackfilt[i] = NULL;
	}
    if ( gfc->inbuf_old[0] ) { 
        free ( gfc->inbuf_old[0] );
	gfc->inbuf_old[0] = NULL;
    }
    if ( gfc->inbuf_old[1] ) { 
        free ( gfc->inbuf_old[1] );
	gfc->inbuf_old[1] = NULL;
    }

    if ( gfc->bs.buf != NULL ) {
        free ( gfc->bs.buf );
        gfc->bs.buf = NULL;
    }

    if ( gfc->VBR_seek_table.bag ) {
        free ( gfc->VBR_seek_table.bag );
        gfc->VBR_seek_table.bag=NULL;
        gfc->VBR_seek_table.size=0;
    }
    if ( gfc->ATH ) {
        free ( gfc->ATH );
    }
    if ( gfc->PSY ) {
        free ( gfc->PSY );
    }
    if ( gfc->rgdata ) {
        free ( gfc->rgdata );
    }
    if ( gfc->s3_ll ) {
        /* XXX allocated in psymodel_init() */
        free ( gfc->s3_ll );
    }
    if ( gfc->s3_ss ) {
        /* XXX allocated in psymodel_init() */
        free ( gfc->s3_ss );
    }
    if ( gfc->in_buffer_0 ) {
        free ( gfc->in_buffer_0 );
    }
    if ( gfc->in_buffer_1 ) {
        free ( gfc->in_buffer_1 );
    }

    free ( gfc );
}



/*those ATH formulas are returning
their minimum value for input = -1*/

FLOAT ATHformula_GB(FLOAT f, FLOAT value)
{
  /* from Painter & Spanias
    modified by Gabriel Bouvigne to better fit the reality
  ath =    3.640 * pow(f,-0.8)
         - 6.800 * exp(-0.6*pow(f-3.4,2.0))
         + 6.000 * exp(-0.15*pow(f-8.7,2.0))
         + 0.6* 0.001 * pow(f,4.0);


  In the past LAME was using the Painter &Spanias formula.
  But we had some recurrent problems with HF content.
  We measured real ATH values, and found the older formula
  to be inacurate in the higher part. So we made this new
  formula and this solved most of HF problematic testcases.
  The tradeoff is that in VBR mode it increases a lot the
  bitrate.*/


/*this curve can be udjusted according to the VBR scale:
it adjusts from something close to Painter & Spanias
on V9 up to Bouvigne's formula for V0. This way the VBR
bitrate is more balanced according to the -V value.*/

  FLOAT ath;

  if (f < -.3)
      f=3410;

  f /= 1000;  /* convert to khz */
  f  = Max(0.01, f);
/*  f  = Min(21.0, f);
*/
  ath =    3.640 * pow(f,-0.8)
         - 6.800 * exp(-0.6*pow(f-3.4,2.0))
         + 6.000 * exp(-0.15*pow(f-8.7,2.0))
         + (0.6+0.04*value)* 0.001 * pow(f,4.0);
  return ath;
}



FLOAT ATHformula(FLOAT f,lame_global_flags *gfp)
{
  switch(gfp->ATHtype)
    {
    case 0:
      return ATHformula_GB(f, 9);
    case 1:
        return ATHformula_GB(f, -1); /*over sensitive, should probably be removed*/
    case 2:
      return ATHformula_GB(f, 0);
    case 3:
      return ATHformula_GB(f, 1) + 6;     /*modification of GB formula by Roel*/
    case 4:
      return ATHformula_GB(f,gfp->ATHcurve);
    }

  return ATHformula_GB(f, 0);
}

/* see for example "Zwicker: Psychoakustik, 1982; ISBN 3-540-11401-7 */
FLOAT freq2bark(FLOAT freq)
{
  /* input: freq in hz  output: barks */
    if (freq<0) freq=0;
    freq = freq * 0.001;
    return 13.0*atan(.76*freq) + 3.5*atan(freq*freq/(7.5*7.5));
}

/* see for example "Zwicker: Psychoakustik, 1982; ISBN 3-540-11401-7 */
FLOAT freq2cbw(FLOAT freq)
{
  /* input: freq in hz  output: critical band width */
    freq = freq * 0.001;
    return 25+75*pow(1+1.4*(freq*freq),0.69);
}






#define ABS(A) (((A)>0) ? (A) : -(A))

int FindNearestBitrate(
int bRate,        /* legal rates from 8 to 320 */
int version)      /* MPEG-1 or MPEG-2 LSF */
{
    int  bitrate = 0;
    int  i;
  
    for ( i = 1; i <= 14; i++ )
        if ( ABS (bitrate_table[version][i] - bRate) < ABS (bitrate - bRate) )
            bitrate = bitrate_table [version] [i];
	    
    return bitrate;
}





#ifndef Min
#define         Min(A, B)       ((A) < (B) ? (A) : (B))
#endif
#ifndef Max
#define         Max(A, B)       ((A) > (B) ? (A) : (B))
#endif


/* Used to find table index when
 * we need bitrate-based values
 * determined using tables
 *
 * bitrate in kbps
 *
 * Gabriel Bouvigne 2002-11-03
 */
int nearestBitrateFullIndex(const int bitrate)
{
    /* borrowed from DM abr presets*/    

    int index; /* resolved range */

    const int bitrate_table[] = {8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};


    int lower_range = 0, lower_range_kbps = 0,
        upper_range = 0, upper_range_kbps = 0;
    

    int b;


    /* We assume specified bitrate will be 320kbps */
    upper_range_kbps = bitrate_table[16];
    upper_range = 16;
    lower_range_kbps = bitrate_table[16];
    lower_range = 16;
 
    /* Determine which significant bitrates the value specified falls between,
     * if loop ends without breaking then we were correct above that the value was 320
     */
    for (b = 0; b < 16; b++) {
        if ((Max(bitrate, bitrate_table[b+1])) != bitrate) {
              upper_range_kbps = bitrate_table[b+1];
              upper_range = b+1;
              lower_range_kbps = bitrate_table[b];
              lower_range = (b);
              break; /* We found upper range */
        }
    }

    /* Determine which range the value specified is closer to */
    if ((upper_range_kbps - bitrate) > (bitrate - lower_range_kbps))
        index = lower_range;
    else
        index = upper_range;

    return index;
}





/* map frequency to a valid MP3 sample frequency
 *
 * Robert Hegemann 2000-07-01
 */
int map2MP3Frequency(int freq)
{
    if (freq <=  8000) return  8000;
    if (freq <= 11025) return 11025;
    if (freq <= 12000) return 12000;
    if (freq <= 16000) return 16000;
    if (freq <= 22050) return 22050;
    if (freq <= 24000) return 24000;
    if (freq <= 32000) return 32000;
    if (freq <= 44100) return 44100;
    
    return 48000;
}

int BitrateIndex(
int bRate,        /* legal rates from 32 to 448 kbps */
int version,      /* MPEG-1 or MPEG-2/2.5 LSF */
int samplerate)   /* convert bitrate in kbps to index */
{
    int  i;

    for ( i = 0; i <= 14; i++)
        if ( bitrate_table [version] [i] == bRate )
            return i;
	    
    return -1;
}

/* convert samp freq in Hz to index */

int SmpFrqIndex ( int sample_freq, int* const version )
{
    switch ( sample_freq ) {
    case 44100: *version = 1; return  0;
    case 48000: *version = 1; return  1;
    case 32000: *version = 1; return  2;
    case 22050: *version = 0; return  0;
    case 24000: *version = 0; return  1;
    case 16000: *version = 0; return  2;
    case 11025: *version = 0; return  0;
    case 12000: *version = 0; return  1;
    case  8000: *version = 0; return  2;
    default:    *version = 0; return -1;
    }
}


/*****************************************************************************
*
*  End of bit_stream.c package
*
*****************************************************************************/










/* resampling via FIR filter, blackman window */
inline static FLOAT blackman(FLOAT x,FLOAT fcn,int l)
{
  /* This algorithm from:
SIGNAL PROCESSING ALGORITHMS IN FORTRAN AND C
S.D. Stearns and R.A. David, Prentice-Hall, 1992
  */
  FLOAT bkwn,x2;
  FLOAT wcn = (PI * fcn);
  
  x /= l;
  if (x<0) x=0;
  if (x>1) x=1;
  x2 = x - .5;

  bkwn = 0.42 - 0.5*cos(2*x*PI)  + 0.08*cos(4*x*PI);
  if (fabs(x2)<1e-9) return wcn/PI;
  else 
    return  (  bkwn*sin(l*wcn*x2)  / (PI*l*x2)  );


}

/* gcd - greatest common divisor */
/* Joint work of Euclid and M. Hendry */

int gcd ( int i, int j )
{
/*    assert ( i > 0  &&  j > 0 ); */
    return j ? gcd(j, i % j) : i;
}



/* copy in new samples from in_buffer into mfbuf, with resampling
   if necessary.  n_in = number of samples from the input buffer that
   were used.  n_out = number of samples copied into mfbuf  */

void fill_buffer(lame_global_flags *gfp,
		 sample_t *mfbuf[2],
		 sample_t *in_buffer[2],
		 int nsamples, int *n_in, int *n_out)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    int ch,i;

    /* copy in new samples into mfbuf, with resampling if necessary */
    if ( (gfc->resample_ratio < .9999) || (gfc->resample_ratio > 1.0001) ){
	for (ch = 0; ch < gfc->channels_out; ch++) {
	    *n_out =
		fill_buffer_resample(gfp, &mfbuf[ch][gfc->mf_size],
				     gfp->framesize, in_buffer[ch],
				     nsamples, n_in, ch);
	}
    }
    else {
	*n_out = Min(gfp->framesize, nsamples);
	*n_in = *n_out;
	for (i = 0; i < *n_out; ++i) {
	    mfbuf[0][gfc->mf_size + i] = in_buffer[0][i];
	    if (gfc->channels_out == 2)
		mfbuf[1][gfc->mf_size + i] = in_buffer[1][i];
	}
    }
}
    



int fill_buffer_resample(
       lame_global_flags *gfp,
       sample_t *outbuf,
       int desired_len,
       sample_t *inbuf,
       int len,
       int *num_used,
       int ch) 
{

  
  lame_internal_flags *gfc=gfp->internal_flags;
  int BLACKSIZE;
  FLOAT offset,xvalue;
  int i,j=0,k;
  int filter_l;
  FLOAT fcn,intratio;
  FLOAT *inbuf_old;
  int bpc;   /* number of convolution functions to pre-compute */
  bpc = gfp->out_samplerate/gcd(gfp->out_samplerate,gfp->in_samplerate);
  if (bpc>BPC) bpc = BPC;

  intratio=( fabs(gfc->resample_ratio - floor(.5+gfc->resample_ratio)) < .0001 );
  fcn = 1.00/gfc->resample_ratio;
  if (fcn>1.00) fcn=1.00;
  filter_l = gfp->quality < 7 ? 31 : 7;
  filter_l = 31;
  if (0==filter_l % 2 ) --filter_l;/* must be odd */
  filter_l += intratio;            /* unless resample_ratio=int, it must be even */


  BLACKSIZE = filter_l+1;  /* size of data needed for FIR */
  
  if ( gfc->fill_buffer_resample_init == 0 ) {
    gfc->inbuf_old[0]=calloc(BLACKSIZE,sizeof(gfc->inbuf_old[0][0]));
    gfc->inbuf_old[1]=calloc(BLACKSIZE,sizeof(gfc->inbuf_old[0][0]));
    for (i=0; i<=2*bpc; ++i)
      gfc->blackfilt[i]=calloc(BLACKSIZE,sizeof(gfc->blackfilt[0][0]));

    gfc->itime[0]=0;
    gfc->itime[1]=0;

    /* precompute blackman filter coefficients */
    for ( j = 0; j <= 2*bpc; j++ ) {
        FLOAT sum = 0.; 
        offset = (j-bpc) / (2.*bpc);
        for ( i = 0; i <= filter_l; i++ ) 
            sum += 
	    gfc->blackfilt[j][i]  = blackman(i-offset,fcn,filter_l);
	for ( i = 0; i <= filter_l; i++ ) 
	  gfc->blackfilt[j][i] /= sum;
    }
    gfc->fill_buffer_resample_init = 1;
  }
  
  inbuf_old=gfc->inbuf_old[ch];

  /* time of j'th element in inbuf = itime + j/ifreq; */
  /* time of k'th element in outbuf   =  j/ofreq */
  for (k=0;k<desired_len;k++) {
    FLOAT time0;
    int joff;

    time0 = k*gfc->resample_ratio;       /* time of k'th output sample */
    j = floor( time0 -gfc->itime[ch]  );

    /* check if we need more input data */
    if ((filter_l + j - filter_l/2) >= len) break;

    /* blackman filter.  by default, window centered at j+.5(filter_l%2) */
    /* but we want a window centered at time0.   */
    offset = ( time0 -gfc->itime[ch] - (j + .5*(filter_l%2)));
    assert(fabs(offset)<=.501);

    /* find the closest precomputed window for this offset: */
    joff = floor((offset*2*bpc) + bpc +.5);

    xvalue = 0.;
    for (i=0 ; i<=filter_l ; ++i) {
      int j2 = i+j-filter_l/2;
      sample_t y;
      assert(j2<len);
      assert(j2+BLACKSIZE >= 0);
      y = (j2<0) ? inbuf_old[BLACKSIZE+j2] : inbuf[j2];
#define PRECOMPUTE
#ifdef PRECOMPUTE
      xvalue += y*gfc->blackfilt[joff][i];
#else
      xvalue += y*blackman(i-offset,fcn,filter_l);  /* very slow! */
#endif
    }
    outbuf[k]=xvalue;
  }

  
  /* k = number of samples added to outbuf */
  /* last k sample used data from [j-filter_l/2,j+filter_l-filter_l/2]  */

  /* how many samples of input data were used:  */
  *num_used = Min(len,filter_l+j-filter_l/2);

  /* adjust our input time counter.  Incriment by the number of samples used,
   * then normalize so that next output sample is at time 0, next
   * input buffer is at time itime[ch] */
  gfc->itime[ch] += *num_used - k*gfc->resample_ratio;

  /* save the last BLACKSIZE samples into the inbuf_old buffer */
  if (*num_used >= BLACKSIZE) {
      for (i=0;i<BLACKSIZE;i++)
	  inbuf_old[i]=inbuf[*num_used + i -BLACKSIZE];
  }else{
      /* shift in *num_used samples into inbuf_old  */
       int n_shift = BLACKSIZE-*num_used;  /* number of samples to shift */

       /* shift n_shift samples by *num_used, to make room for the
	* num_used new samples */
       for (i=0; i<n_shift; ++i ) 
	   inbuf_old[i] = inbuf_old[i+ *num_used];

       /* shift in the *num_used samples */
       for (j=0; i<BLACKSIZE; ++i, ++j ) 
	   inbuf_old[i] = inbuf[j];

       assert(j==*num_used);
  }
  return k;  /* return the number samples created at the new samplerate */
}





/***********************************************************************
*
*  Message Output
*
***********************************************************************/
void  lame_debugf (const lame_internal_flags *gfc, const char* format, ... )
{
    va_list  args;

    va_start ( args, format );

    if ( gfc->report.debugf != NULL ) {
        gfc->report.debugf( format, args );
    } else {
        (void) vfprintf ( stderr, format, args );
        fflush ( stderr );      /* an debug function should flush immediately */
    }

    va_end   ( args );
}


void  lame_msgf (const lame_internal_flags *gfc, const char* format, ... )
{
    va_list  args;

    va_start ( args, format );
   
    if ( gfc->report.msgf != NULL ) {
        gfc->report.msgf( format, args );
    } else {
        (void) vfprintf ( stderr, format, args );
        fflush ( stderr );     /* we print to stderr, so me may want to flush */
    }

    va_end   ( args );
}


void  lame_errorf (const lame_internal_flags *gfc, const char* format, ... )
{
    va_list  args;

    va_start ( args, format );
    
    if ( gfc->report.errorf != NULL ) {
        gfc->report.errorf( format, args );
    } else {
        (void) vfprintf ( stderr, format, args );
        fflush   ( stderr );    /* an error function should flush immediately */
    }

    va_end   ( args );
}



/***********************************************************************
 *
 *      routines to detect CPU specific features like 3DNow, MMX, SSE
 *
 *  donated by Frank Klemm
 *  added Robert Hegemann 2000-10-10
 *
 ***********************************************************************/


int  has_MMX ( void )
{
#ifdef HAVE_NASM 
    extern int has_MMX_nasm ( void );
    return has_MMX_nasm ();
#else
    return 0;   /* don't know, assume not */
#endif
}    

int  has_3DNow ( void )
{
#ifdef HAVE_NASM 
    extern int has_3DNow_nasm ( void );
    return has_3DNow_nasm ();
#else
    return 0;   /* don't know, assume not */
#endif
}    

int  has_SSE ( void )
{
#ifdef HAVE_NASM 
    extern int has_SSE_nasm ( void );
    return has_SSE_nasm ();
#else
    return 0;   /* don't know, assume not */
#endif
}    

int  has_SSE2 ( void )
{
#ifdef HAVE_NASM 
    extern int has_SSE2_nasm ( void );
    return has_SSE2_nasm ();
#else
    return 0;   /* don't know, assume not */
#endif
}    

void disable_FPE(void) {
/* extremly system dependent stuff, move to a lib to make the code readable */
/*==========================================================================*/



    /*
     *  Disable floating point exceptions
     */




#if defined(__FreeBSD__) && !defined(__alpha__)
    {
        /* seet floating point mask to the Linux default */
        fp_except_t mask;
        mask = fpgetmask();
        /* if bit is set, we get SIGFPE on that error! */
        fpsetmask(mask & ~(FP_X_INV | FP_X_DZ));
        /*  DEBUGF("FreeBSD mask is 0x%x\n",mask); */
    }
#endif

#if defined(__riscos__) && !defined(ABORTFP)
    /* Disable FPE's under RISC OS */
    /* if bit is set, we disable trapping that error! */
    /*   _FPE_IVO : invalid operation */
    /*   _FPE_DVZ : divide by zero */
    /*   _FPE_OFL : overflow */
    /*   _FPE_UFL : underflow */
    /*   _FPE_INX : inexact */
    DisableFPETraps(_FPE_IVO | _FPE_DVZ | _FPE_OFL);
#endif

    /*
     *  Debugging stuff
     *  The default is to ignore FPE's, unless compiled with -DABORTFP
     *  so add code below to ENABLE FPE's.
     */

#if defined(ABORTFP)
#if defined(_MSC_VER)
    {

   /* set affinity to a single CPU.  Fix for EAC/lame on SMP systems from
     "Todd Richmond" <todd.richmond@openwave.com> */
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    SetProcessAffinityMask(GetCurrentProcess(), si.dwActiveProcessorMask);

#include <float.h>
        unsigned int mask;
        mask = _controlfp(0, 0);
        mask &= ~(_EM_OVERFLOW | _EM_UNDERFLOW | _EM_ZERODIVIDE | _EM_INVALID);
        mask = _controlfp(mask, _MCW_EM);
    }
#elif defined(__CYGWIN__)
#  define _FPU_GETCW(cw) __asm__ ("fnstcw %0" : "=m" (*&cw))
#  define _FPU_SETCW(cw) __asm__ ("fldcw %0" : : "m" (*&cw))

#  define _EM_INEXACT     0x00000020 /* inexact (precision) */
#  define _EM_UNDERFLOW   0x00000010 /* underflow */
#  define _EM_OVERFLOW    0x00000008 /* overflow */
#  define _EM_ZERODIVIDE  0x00000004 /* zero divide */
#  define _EM_INVALID     0x00000001 /* invalid */
    {
        unsigned int mask;
        _FPU_GETCW(mask);
        /* Set the FPU control word to abort on most FPEs */
        mask &= ~(_EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID);
        _FPU_SETCW(mask);
    }
# elif defined(__linux__)
    {

#  include <fpu_control.h>
#  ifndef _FPU_GETCW
#  define _FPU_GETCW(cw) __asm__ ("fnstcw %0" : "=m" (*&cw))
#  endif
#  ifndef _FPU_SETCW
#  define _FPU_SETCW(cw) __asm__ ("fldcw %0" : : "m" (*&cw))
#  endif

        /* 
         * Set the Linux mask to abort on most FPE's
         * if bit is set, we _mask_ SIGFPE on that error!
         *  mask &= ~( _FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM | _FPU_MASK_UM );
         */

        unsigned int mask;
        _FPU_GETCW(mask);
        mask &= ~(_FPU_MASK_IM | _FPU_MASK_ZM | _FPU_MASK_OM);
        _FPU_SETCW(mask);
    }
#endif
#endif /* ABORTFP */
}





#ifdef USE_FAST_LOG
/***********************************************************************
 *
 * Fast Log Approximation for log2, used to approximate every other log
 * (log10 and log)
 * maximum absolute error for log10 is around 10-6
 * maximum *relative* error can be high when x is almost 1 because error/log10(x) tends toward x/e
 *
 * use it if typical RESULT values are > 1e-5 (for example if x>1.00001 or x<0.99999)
 * or if the relative precision in the domain around 1 is not important (result in 1 is exact and 0)
 *
 ***********************************************************************/


#define LOG2_SIZE       (512)
#define LOG2_SIZE_L2    (9)

static ieee754_float32_t log_table[LOG2_SIZE+1];



void init_log_table(void)
{
  int j;
  static int init = 0;

  /* Range for log2(x) over [1,2[ is [0,1[ */
  assert((1<<LOG2_SIZE_L2)==LOG2_SIZE);
  
  if(!init) {
    for(j=0; j<LOG2_SIZE+1; j++)
      log_table[j] = log(1.0f+j/(ieee754_float32_t)LOG2_SIZE)/log(2.0f);
  }
  init = 1;
}



ieee754_float32_t fast_log2(ieee754_float32_t x)
{
  ieee754_float32_t log2val, partial;
  union {
    ieee754_float32_t f;
    int     i;
  } fi;
  int mantisse;
  fi.f = x;
  mantisse = fi.i & 0x7fffff;
  log2val = ((fi.i>>23) & 0xFF)-0x7f;
  partial = (mantisse & ((1<<(23-LOG2_SIZE_L2))-1));
  partial *= 1.0f/((1<<(23-LOG2_SIZE_L2)));


  mantisse >>= (23-LOG2_SIZE_L2);

  /* log2val += log_table[mantisse];  without interpolation the results are not good */
  log2val += log_table[mantisse] * (1.0f-partial) + log_table[mantisse+1]*partial;

  return log2val;
}

#else /* Don't use FAST_LOG */


void init_log_table(void)
{
}


#endif

/* end of util.c */

