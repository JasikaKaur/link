/* rsa.c  -  RSA function
 *	Copyright (c) 1997,1998,1999 by Werner Koch (dd9jn)
 ***********************************************************************
 * ATTENTION: This code should not be exported to the United States
 * nor should it be used there without a license agreement with PKP.
 * The RSA algorithm is protected by U.S. Patent #4,405,829 which
 * expires on September 20, 2000!
 ***********************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * WERNER KOCH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Werner Koch shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Werner Koch.
 */

/* How to compile:
 *
	 gcc -Wall -O2 -shared -fPIC -o rsa rsa.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* configuration stuff */
#ifdef __alpha__
  #define SIZEOF_UNSIGNED_LONG 8
#else
  #define SIZEOF_UNSIGNED_LONG 4
#endif

#if defined(__mc68000__) || defined (__sparc__) || defined (__PPC__) \
    || (defined(__mips__) && (defined(MIPSEB) || defined (__MIPSEB__)) ) \
    || defined(__hpux__)  /* should be replaced by the macro for the PA */
  #define BIG_ENDIAN_HOST 1
#else
  #define LITTLE_ENDIAN_HOST 1
#endif

typedef unsigned long  ulong;
typedef unsigned short ushort;
typedef unsigned char  byte;

typedef unsigned short u16;
typedef unsigned long  u32;

/* end configurable stuff */


const char * const gnupgext_version = "RSA ($Revision: 1.10 $)";

#ifndef DIM
  #define DIM(v) (sizeof(v)/sizeof((v)[0]))
  #define DIMof(type,member)   DIM(((type *)0)->member)
#endif

#define is_RSA(a) ((a)>=1 && (a)<=3)

#define BAD_ALGO  4
#define BAD_KEY   7
#define BAD_SIGN  8

struct mpi_struct { int hidden_stuff; };
typedef struct mpi_struct *MPI;

extern int g10c_debug_mode;
extern int g10_opt_verbose;



typedef struct {
    MPI n;	    /* modulus */
    MPI e;	    /* exponent */
} RSA_public_key;


typedef struct {
    MPI n;	    /* public modulus */
    MPI e;	    /* public exponent */
    MPI d;	    /* exponent */
    MPI p;	    /* prime  p. */
    MPI q;	    /* prime  q. */
    MPI u;	    /* inverse of p mod q. */
} RSA_secret_key;

/* imports */
void *g10_malloc( size_t n );
void *g10_calloc( size_t n );
void g10_free( void *p);
void g10_log_fatal( const char *fmt, ... );
void g10_log_error( const char *fmt, ... );
void g10_log_info( const char *fmt, ... );
void g10_log_debug( const char *fmt, ... );
void g10_log_hexdump( const char *text, char *buf, size_t len );
void g10_log_mpidump( const char *text, MPI a );
MPI  g10c_generate_secret_prime( unsigned nbits );
char *g10c_get_random_bits( unsigned nbits, int level, int secure );
MPI  g10m_new( unsigned nbits );
MPI  g10m_new_secure( unsigned nbits );
void g10m_release( MPI a );
void g10m_set( MPI w, MPI u);
void g10m_set_ui( MPI w, unsigned long u);
void g10m_set_buffer( MPI a, const char *buffer, unsigned nbytes, int sign );
MPI  g10m_copy( MPI a );
void g10m_swap( MPI a, MPI b);
unsigned g10m_get_nbits( MPI a );
unsigned g10m_get_size( MPI a );
int  g10m_cmp( MPI u, MPI v );
void g10m_add_ui(MPI w, MPI u, unsigned long v );
void g10m_sub_ui(MPI w, MPI u, unsigned long v );
void g10m_mul( MPI w, MPI u, MPI v);
void g10m_fdiv_q( MPI quot, MPI dividend, MPI divisor );
void g10m_powm( MPI res, MPI base, MPI exp, MPI mod);
int  g10m_gcd( MPI g, MPI a, MPI b );
int  g10m_invm( MPI x, MPI u, MPI v );

/* local prototypes */
static void test_keys( RSA_secret_key *sk, unsigned nbits );
static void generate( RSA_secret_key *sk, unsigned nbits );
static int  check_secret_key( RSA_secret_key *sk );
static void public(MPI output, MPI input, RSA_public_key *skey );
static void secret(MPI output, MPI input, RSA_secret_key *skey );


static void
test_keys( RSA_secret_key *sk, unsigned nbits )
{
    RSA_public_key pk;
    MPI test = g10m_new( nbits );
    MPI out1 = g10m_new( nbits );
    MPI out2 = g10m_new( nbits );

    pk.n = sk->n;
    pk.e = sk->e;
    {	char *p = g10c_get_random_bits( nbits, 0, 0 );
	g10m_set_buffer( test, p, (nbits+7)/8, 0 );
	g10_free(p);
    }

    public( out1, test, &pk );
    secret( out2, out1, sk );
    if( g10m_cmp( test, out2 ) )
	g10_log_fatal("RSA operation: public, secret failed\n");
    secret( out1, test, sk );
    public( out2, out1, &pk );
    if( g10m_cmp( test, out2 ) )
	g10_log_fatal("RSA operation: secret, public failed\n");
    g10m_release( test );
    g10m_release( out1 );
    g10m_release( out2 );
}

/****************
 * Generate a key pair with a key of size NBITS
 * Returns: 2 structures filles with all needed values
 */
static void
generate( RSA_secret_key *sk, unsigned nbits )
{
    MPI p, q; /* the two primes */
    MPI d;    /* the private key */
    MPI u;
    MPI t1, t2;
    MPI n;    /* the public key */
    MPI e;    /* the exponent */
    MPI phi;  /* helper: (p-a)(q-1) */
    MPI g;
    MPI f;

    /* select two (very secret) primes */
    p = g10c_generate_secret_prime( nbits / 2 );
    q = g10c_generate_secret_prime( nbits / 2 );
    if( g10m_cmp( p, q ) > 0 ) /* p shall be smaller than q (for calc of u)*/
	g10m_swap(p,q);
    /* calculate Euler totient: phi = (p-1)(q-1) */
    t1 = g10m_new_secure( g10m_get_size(p) );
    t2 = g10m_new_secure( g10m_get_size(p) );
    phi = g10m_new_secure( nbits );
    g	= g10m_new_secure( nbits  );
    f	= g10m_new_secure( nbits  );
    g10m_sub_ui( t1, p, 1 );
    g10m_sub_ui( t2, q, 1 );
    g10m_mul( phi, t1, t2 );
    g10m_gcd(g, t1, t2);
    g10m_fdiv_q(f, phi, g);
    /* multiply them to make the private key */
    n = g10m_new( nbits );
    g10m_mul( n, p, q );
    /* find a public exponent  */
    e = g10m_new(6);
    g10m_set_ui( e, 17); /* start with 17 */
    while( !g10m_gcd(t1, e, phi) ) /* (while gcd is not 1) */
	g10m_add_ui( e, e, 2);
    /* calculate the secret key d = e^1 mod phi */
    d = g10m_new( nbits );
    g10m_invm(d, e, f );
    /* calculate the inverse of p and q (used for chinese remainder theorem)*/
    u = g10m_new( nbits );
    g10m_invm(u, p, q );

    if( g10c_debug_mode ) {
	g10_log_mpidump("  p= ", p );
	g10_log_mpidump("  q= ", q );
	g10_log_mpidump("phi= ", phi );
	g10_log_mpidump("  g= ", g );
	g10_log_mpidump("  f= ", f );
	g10_log_mpidump("  n= ", n );
	g10_log_mpidump("  e= ", e );
	g10_log_mpidump("  d= ", d );
	g10_log_mpidump("  u= ", u );
    }

    g10m_release(t1);
    g10m_release(t2);
    g10m_release(phi);
    g10m_release(f);
    g10m_release(g);

    sk->n = n;
    sk->e = e;
    sk->p = p;
    sk->q = q;
    sk->d = d;
    sk->u = u;

    /* now we can test our keys (this should never fail!) */
    test_keys( sk, nbits - 64 );
}


/****************
 * Test wether the secret key is valid.
 * Returns: true if this is a valid key.
 */
static int
check_secret_key( RSA_secret_key *sk )
{
    int rc;
    MPI temp = g10m_new( g10m_get_size(sk->p)*2 );

    g10m_mul(temp, sk->p, sk->q );
    rc = g10m_cmp( temp, sk->n );
    g10m_release(temp);
    return !rc;
}



/****************
 * Public key operation. Encrypt INPUT with PKEY and put result into OUTPUT.
 *
 *	c = m^e mod n
 *
 * Where c is OUTPUT, m is INPUT and e,n are elements of PKEY.
 */
static void
public(MPI output, MPI input, RSA_public_key *pkey )
{
    if( output == input ) { /* powm doesn't like output and input the same */
	MPI x = g10m_new( g10m_get_size(input)*2 );
	g10m_powm( x, input, pkey->e, pkey->n );
	g10m_set(output, x);
	g10m_release(x);
    }
    else
	g10m_powm( output, input, pkey->e, pkey->n );
}

/****************
 * Secret key operation. Encrypt INPUT with SKEY and put result into OUTPUT.
 *
 *	m = c^d mod n
 *
 * Where m is OUTPUT, c is INPUT and d,n are elements of PKEY.
 *
 * FIXME: We should better use the Chinese Remainder Theorem
 */
static void
secret(MPI output, MPI input, RSA_secret_key *skey )
{
    g10m_powm( output, input, skey->d, skey->n );
}


/*********************************************
 **************  interface  ******************
 *********************************************/

static int
do_generate( int algo, unsigned nbits, MPI *skey, MPI **retfactors )
{
    RSA_secret_key sk;

    if( !is_RSA(algo) )
	return BAD_ALGO;

    generate( &sk, nbits );
    skey[0] = sk.n;
    skey[1] = sk.e;
    skey[2] = sk.d;
    skey[3] = sk.p;
    skey[4] = sk.q;
    skey[5] = sk.u;
    /* make an empty list of factors */
    *retfactors = g10_calloc( 1 * sizeof **retfactors );
    return 0;
}


static int
do_check_secret_key( int algo, MPI *skey )
{
    RSA_secret_key sk;

    if( !is_RSA(algo) )
	return BAD_ALGO;

    sk.n = skey[0];
    sk.e = skey[1];
    sk.d = skey[2];
    sk.p = skey[3];
    sk.q = skey[4];
    sk.u = skey[5];
    if( !check_secret_key( &sk ) )
	return BAD_KEY;

    return 0;
}



static int
do_encrypt( int algo, MPI *resarr, MPI data, MPI *pkey )
{
    RSA_public_key pk;

    if( algo != 1 && algo != 2 )
	return BAD_ALGO;

    pk.n = pkey[0];
    pk.e = pkey[1];
    resarr[0] = g10m_new( g10m_get_size( pk.n ) );
    public( resarr[0], data, &pk );
    return 0;
}

static int
do_decrypt( int algo, MPI *result, MPI *data, MPI *skey )
{
    RSA_secret_key sk;

    if( algo != 1 && algo != 2 )
	return BAD_ALGO;

    sk.n = skey[0];
    sk.e = skey[1];
    sk.d = skey[2];
    sk.p = skey[3];
    sk.q = skey[4];
    sk.u = skey[5];
    *result = g10m_new_secure( g10m_get_size( sk.n ) );
    secret( *result, data[0], &sk );
    return 0;
}

static int
do_sign( int algo, MPI *resarr, MPI data, MPI *skey )
{
    RSA_secret_key sk;

    if( algo != 1 && algo != 3 )
	return BAD_ALGO;

    sk.n = skey[0];
    sk.e = skey[1];
    sk.d = skey[2];
    sk.p = skey[3];
    sk.q = skey[4];
    sk.u = skey[5];
    resarr[0] = g10m_new( g10m_get_size( sk.n ) );
    secret( resarr[0], data, &sk );

    return 0;
}

static int
do_verify( int algo, MPI hash, MPI *data, MPI *pkey,
	   int (*cmp)(void *opaque, MPI tmp), void *opaquev )
{
    RSA_public_key pk;
    MPI result;
    int rc;

    if( algo != 1 && algo != 3 )
	return BAD_ALGO;
    pk.n = pkey[0];
    pk.e = pkey[1];
    result = g10m_new(160);
    public( result, data[0], &pk );
    /*rc = (*cmp)( opaquev, result );*/
    rc = g10m_cmp( result, hash )? BAD_SIGN:0;
    g10m_release(result);

    return rc;
}


static unsigned
do_get_nbits( int algo, MPI *pkey )
{
    if( !is_RSA(algo) )
	return 0;
    return g10m_get_nbits( pkey[0] );
}


/****************
 * Return some information about the algorithm.  We need algo here to
 * distinguish different flavors of the algorithm.
 * Returns: A pointer to string describing the algorithm or NULL if
 *	    the ALGO is invalid.
 * Usage: Bit 0 set : allows signing
 *	      1 set : allows encryption
 */
static const char *
rsa_get_info( int algo,
	      int *npkey, int *nskey, int *nenc, int *nsig, int *usage,
    int (**r_generate)( int algo, unsigned nbits, MPI *skey, MPI **retfactors ),
    int (**r_check_secret_key)( int algo, MPI *skey ),
    int (**r_encrypt)( int algo, MPI *resarr, MPI data, MPI *pkey ),
    int (**r_decrypt)( int algo, MPI *result, MPI *data, MPI *skey ),
    int (**r_sign)( int algo, MPI *resarr, MPI data, MPI *skey ),
    int (**r_verify)( int algo, MPI hash, MPI *data, MPI *pkey,
				    int (*)(void *, MPI), void *),
    unsigned (**r_get_nbits)( int algo, MPI *pkey ) )
{
    *npkey = 2;
    *nskey = 6;
    *nenc = 1;
    *nsig = 1;
    *r_generate 	= do_generate	     ;
    *r_check_secret_key = do_check_secret_key;
    *r_encrypt		= do_encrypt	     ;
    *r_decrypt		= do_decrypt	     ;
    *r_sign		= do_sign	     ;
    *r_verify		= do_verify	     ;
    *r_get_nbits	= do_get_nbits	     ;

    switch( algo ) {
      case 1: *usage = 2|1; return "RSA";
      case 2: *usage = 2  ; return "RSA-E";
      case 3: *usage =	 1; return "RSA-S";
      default:*usage = 0; return NULL;
    }
}


static struct {
    int class;
    int version;
    int  value;
    void (*func)(void);
} func_table[] = {
    { 30, 1, 0, (void(*)(void))rsa_get_info },
    { 31, 1, 1 }, /* RSA */
    { 31, 1, 2 }, /* RSA encrypt only */
    { 31, 1, 3 }, /* RSA sign only */
};



/****************
 * Enumerate the names of the functions together with informations about
 * this function. Set sequence to an integer with a initial value of 0 and
 * do not change it.
 * If what is 0 all kind of functions are returned.
 * Return values: class := class of function:
 *			   10 = message digest algorithm info function
 *			   11 = integer with available md algorithms
 *			   20 = cipher algorithm info function
 *			   21 = integer with available cipher algorithms
 *			   30 = public key algorithm info function
 *			   31 = integer with available pubkey algorithms
 *		  version = interface version of the function/pointer
 */
void *
gnupgext_enum_func( int what, int *sequence, int *class, int *vers )
{
    void *ret;
    int i = *sequence;

    do {
	if( i >= DIM(func_table) || i < 0 ) {
	    return NULL;
	}
	*class = func_table[i].class;
	*vers  = func_table[i].version;
	switch( *class ) {
	  case 11:
	  case 21:
	  case 31:
	    ret = &func_table[i].value;
	    break;
	  default:
	    ret = func_table[i].func;
	    break;
	}
	i++;
    } while( what && what != *class );

    *sequence = i;
    return ret;
}


