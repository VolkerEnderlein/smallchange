
#include <assert.h>
// #include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>

#include "tariff.h"
// #include "config.h"

// 20010808 ThH
#include <io.h>
#define TRUE 1
#define FALSE 0

/**************************************************************************/

/* WARNING: peeklen is still hardcoded into the source at various places */

#if RIFF_PEEKLEN != 12
#error "WARNING: riff is still hardcoded for a RIFF_PEEKLEN of 12"
#endif

/**************************************************************************/

unsigned int
char_to_uint(
    unsigned char * buf )
{
    return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
} /* char_to_uint() */

unsigned int
char_to_ushort(
    unsigned char * buf )
{
    return (buf[1] << 8) | buf[0];
} /* char_to_uint() */

/**************************************************************************/

riff_t *
riff_file_open(
    const char * const pathname )
{
    riff_t * handle;
    struct stat sb;

    handle = (riff_t *) malloc( sizeof( riff_t ) );
    if ( handle == NULL ) {
        fprintf( stderr, "malloc error\n" );
        return NULL;
    }

    handle->pos = 0;
    handle->mark = 0;
    handle->size = 0;

    handle->fd = open( pathname, O_RDONLY );
    if ( handle->fd == -1 ) {
        free( handle );
        perror( "riff_open: open()" );
        return NULL;
    }

    if ( fstat( handle->fd, &sb ) == -1 ) {
        close( handle->fd );
        free( handle );
        perror( "riff_open: fstat()" );
        return NULL;
    }
    handle->size = sb.st_size;

    if ( read( handle->fd, handle->magic, 12 ) != 12 ) {
        close( handle->fd );
        free( handle );
        perror( "riff_open: read() magic" );
        return NULL;
    }
    handle->pos = 12;
    
    if ( strncmp( handle->magic, "RIFF", 4 ) != 0 ) {
        close( handle->fd );
        free( handle );
        perror( "riff_open: bad magic" );
        return NULL;
    }

    if ( read( handle->fd, handle->peek, RIFF_PEEKLEN ) != RIFF_PEEKLEN ) {
        close( handle->fd );
        free( handle );
        perror( "riff_open: peek problem" );
        return NULL;
    }
    handle->pos += RIFF_PEEKLEN;

    return handle;
} /* riff_file_open() */

void
riff_file_close(
    riff_t * file )
{
    close( file->fd );
    free( file );
} /* riff_file_close() */

int
riff_file_is_type(
    riff_t * file,
    const char * const type )
{
    if ( strncmp( file->magic + 8, type, 4 ) == 0 )
        return 1;
    return 0;
} /* riff_filetype() */

int
riff_file_size(
    riff_t * file )
{
    return char_to_uint( (unsigned char *) file->magic + 4 );
} /* riff_file_size() */

/**************************************************************************/

void *
riff_chunk_read(
    riff_t * file )
{
    int chunksize;
    void * chunk;

    chunksize = char_to_uint( (unsigned char *) file->peek + 4 );
    if ( chunksize % 2 ) /* chunk aligned on 2 bytes */
        chunksize++;

    chunk = (void *) malloc( chunksize + 8 );
    if ( ! chunk )
        return NULL;

    memcpy( chunk, file->peek, RIFF_PEEKLEN );
    if ( read( file->fd, ((char *) chunk) + RIFF_PEEKLEN, chunksize - 4 )
             != (chunksize - 4) ) {
        free( chunk );
        return NULL;
    }
    file->pos += chunksize - 4;

    /* FIXME: deal with EOF */
    if ( read( file->fd, file->peek, RIFF_PEEKLEN ) != RIFF_PEEKLEN )
        return NULL;

    file->pos += RIFF_PEEKLEN;
    return chunk;
} /* riff_read_chunk() */

int
riff_chunk_read_data(
    riff_t * file,
    char * buf,
    int off )
{
    int size, alignedsize;

    size = char_to_uint( (unsigned char *) file->peek + 4 );
    alignedsize = size;

    if ( alignedsize % 2 ) /* chunk aligned on 2 bytes, but preserve size */
        alignedsize++;

    if ( off <= 4 ) {
        if ( off < 4 )
            memcpy( buf, file->peek + 8 + off, 4 - off );
        if ( read( file->fd, buf + 4 - off, alignedsize - 4 )
                 != (alignedsize - 4) ) {
            fprintf( stderr, "read() failed\n" );
            return 0;
        }
        file->pos += alignedsize - 4;
    } else {
        lseek( file->fd, off - 4, SEEK_CUR );
        file->pos += off - 4;
        if ( read( file->fd, buf, alignedsize - off )
                 != (alignedsize - off) ) {
            fprintf( stderr, "read() failed\n" );
            return 0;
        }
        file->pos += alignedsize - off;
    }

    /* FIXME: deal with EOF */
    if ( read( file->fd, file->peek, RIFF_PEEKLEN ) != RIFF_PEEKLEN ) {
        fprintf( stderr, "read(peek) failed\n" );
        return 0;
    }

    file->pos += RIFF_PEEKLEN;
/*    fprintf( stderr, "size = %d - returned %d\n", size, size - off ); */
    return size - off;
} /* riff_read_chunk_data() */

void
riff_chunk_skip(
    riff_t * file )
{
    int chunksize;
    chunksize = char_to_uint( (unsigned char *) file->peek + 4 );
    lseek( file->fd, chunksize - 4, SEEK_CUR );
    file->pos += chunksize - 4;
    read( file->fd, file->peek, RIFF_PEEKLEN );
    file->pos += RIFF_PEEKLEN;
} /* riff_skip_chunk() */

int
riff_list_enter(
    riff_t * file )
{
    if ( RIFF_PEEKLEN > 12 )
        memmove( file->peek, file->peek + 12, RIFF_PEEKLEN - 12 );
    read( file->fd, file->peek, RIFF_PEEKLEN );
    file->pos += 12;
    return TRUE;
} /* riff_enter_list() */

int
riff_chunk_enter(
    riff_t * file )
{
    if ( RIFF_PEEKLEN > 8 )
        memmove( file->peek, file->peek + 8, RIFF_PEEKLEN - 8 );
    read( file->fd, file->peek + RIFF_PEEKLEN - 8, 8 );
    file->pos += 8;
    return TRUE;
} /* riff_enter_chunk() */

void
riff_filepos_mark(
    riff_t * file )
{
    file->mark = file->pos - RIFF_PEEKLEN;
} /* riff_mark_position() */

void
riff_filepos_goto_mark(
    riff_t * file )
{
    lseek( file->fd, file->mark, SEEK_SET );
    read( file->fd, file->peek, RIFF_PEEKLEN );
    file->pos = file->mark + RIFF_PEEKLEN;
} /* riff_goto_marked_position() */

/**************************************************************************/

int
riff_chunk_is_type(
    void * chunk,
    const char * const type )
{
    if ( strncmp( (char *) chunk, type, 4 ) == 0 )
        return TRUE;
    return FALSE;
} /* riff_chunk_of_type() */

int
riff_chunk_is_list_type(
    void * list,
    const char * const type )
{
    if ( strncmp( (char *) list + 8, type, 4 ) == 0 &&
         strncmp( (char *) list, "LIST", 4 ) == 0 )
        return TRUE;
    return FALSE;
} /* riff_chunk_is_list_type() */

int
riff_chunk_size(
    void * chunk )
{
    return char_to_uint( (unsigned char *) chunk + 4 );
} /* riff_chunk_size() */

int
riff_list_size(
    void * chunk )
{
    return char_to_uint( (unsigned char *) chunk + 4 ) - 4;
} /* riff_chunk_size() */

int
riff_next_chunk_is_type(
    riff_t * file,
    const char * const type )
{
    if ( strncmp( file->peek, type, 4 ) == 0 )
        return TRUE;
    return FALSE;
} /* riff_next_chunk_is_type() */

int
riff_next_chunk_is_list_type(
    riff_t * file,
    const char * const type )
{
    if ( strncmp( file->peek + 8, type, 4 ) == 0 &&
         strncmp( file->peek, "LIST", 4 ) == 0 )
        return TRUE;
    return FALSE;
} /* riff_next_chunk_is_list_type() */

int
riff_next_chunk_size(
    riff_t * file )
{
    return char_to_uint( (unsigned char *) file->peek + 4 );
} /* riff_next_chunk_size() */

int
riff_next_list_size(
    riff_t * file )
{
    return char_to_uint( (unsigned char *) file->peek + 4 ) - 4;
} /* riff_next_chunk_size() */

void
riff_chunk_free(
    void * chunk )
{
    free( chunk );
} /* riff_free_chunk() */

void
riff_list_free(
    void * list )
{
    free( list );
} /* riff_free_list() */

int
riff_chunk_longword(
    void * chunk,
    int word )
{
/*    fprintf( stderr, "fetching longword %d in chunk of size %d\n",
        word, riff_chunk_size( chunk ) ); */
    assert( word >= 0 && word < (riff_chunk_size( chunk ) / 4) );
    return char_to_uint( (unsigned char *) chunk + ((word + 2) * 4));
} /* riff_chunk_longword() */

int
riff_chunk_shortword(
    void * chunk,
    int word )
{
    assert( word >= 0 && word < (riff_chunk_size( chunk ) / 2) );
    return char_to_ushort( (unsigned char *) chunk + ((word + 4) * 2));
} /* riff_chunk_shortword() */

/**************************************************************************/

void
riff_chunk_info(
    void * chunk,
    int words )
{
    unsigned char * data;
    unsigned char * start;
    int size, i;
    
    data = (unsigned char *) chunk;
    size = char_to_uint( data + 4 );
    if ( words == -1 )
        words = size / 4;

    if ( strncmp( (char *) data, "LIST", 4 ) == 0 ) {
/*        fprintf( stderr, "Next item: LIST of type '%c%c%c%c', size %d\n",
            data[8], data[9], data[10], data[11], char_to_uint( data + 4 ) ); */
        start = data + 12;
        if ( ((size / 4) - 1) < words )
            words = (size / 4) - 1;
    } else {
/*        fprintf( stderr, "Next item: CHUNK of type '%c%c%c%c', size %d\n",
            data[0], data[1], data[2], data[3], char_to_uint( data + 4 ) ); */
        start = data + 8;
        if ( (size / 4) < words )
            words = size / 4;
    }
    for ( i = 0; i < words; i++ ) {
        char string[5];
        string[0] = isprint( start[0] ) ? start[0] : '.';
        string[1] = isprint( start[1] ) ? start[1] : '.';
        string[2] = isprint( start[2] ) ? start[2] : '.';
        string[3] = isprint( start[3] ) ? start[3] : '.';
        string[4] = 0;
        printf( "word %2d:  0x%08x  %11d  \"%s\"\n", i,
            char_to_uint( start ), char_to_uint( start ), string );
        start += 4;
    }
} /* riff_chunk_info() */

void
riff_peek_info(
    riff_t * file )
{
    if ( strncmp( file->peek, "LIST", 4 ) == 0 )
        printf( "Next item at pos %d: LIST '%c%c%c%c', size %d\n",
            file->pos - 12, file->peek[8], file->peek[9], file->peek[10],
            file->peek[11], char_to_uint( (unsigned char *) file->peek + 4 ) );
    else
        printf( "Next item at pos %d: CHUNK '%c%c%c%c', size %d\n",
            file->pos - 12, file->peek[0], file->peek[1], file->peek[2],
            file->peek[3], char_to_uint( (unsigned char *) file->peek + 4 ) );
} /* riff_peek_info() */

/**************************************************************************/
