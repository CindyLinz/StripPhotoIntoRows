#include <gd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#define GUESS_ARG_TIMES (100)
#define GUESS_OFFSET_SEP_TIMES (100)
#define GUESS_MAX_ARG (M_PI/10)
#define GUESSED_MAX_ROWS (200)
#define GUESSED_MIN_ROWS (50)
#define USE_THREAD (4)
#define THRESHOLD(color) ((color & 0xFF) < 128)

struct try_data_t {
    int i, error, width, height;
    double arg, offset, sep;
    char * bitmap;
};

inline double getDis(int x, int y, double arg){
    return sqrt(x*x + y*y) * sin(atan2(y, x) - arg);
}

inline int getWeightByDis(double dis, double offset, double sep){
    dis = (dis-offset) / sep;
    dis -= round(dis);
    if( -.1 < dis && dis < .1 )
        return 1;
    else
        return 0;
}

inline int getWeight(int x, int y, double arg, double offset, double sep){
    return getWeightByDis(getDis(x, y, arg), offset, sep);
}

void * try_core(void * d){
    struct try_data_t * data = (struct try_data_t *) d;
    srand(time(0)+data->i);
    int width = data->width;
    int height = data->height;
    char * bitmap = data->bitmap;
    double diag = sqrt( width*width + height*height );
    double darg = atan2(height, width);
    double min_sep = diag / GUESSED_MAX_ROWS;
    double max_sep = diag / GUESSED_MIN_ROWS;
    int best_error = height * width;
    double best_arg, best_offset, best_sep;
    double * dismap = (double*) malloc(width*height*sizeof(double));
    for(int step=0; step<(GUESS_ARG_TIMES+USE_THREAD-1)/USE_THREAD; ++step){
        double arg = (double) rand()/RAND_MAX * (GUESS_MAX_ARG*2) - GUESS_MAX_ARG;

        int i = 0;
        for(int y=0; y<height; ++y)
            for(int x=0; x<width; ++x){
                if( bitmap[i] )
                    dismap[i] = getDis(x, y, arg);
                ++i;
            }
        for(int step2=0; step2<GUESS_OFFSET_SEP_TIMES; ++step2){
            double sep = (double) rand() / RAND_MAX * (max_sep-min_sep) + min_sep;
            double offset = (double) rand() / RAND_MAX * sep;

            int dirty = 0;
            int max_i = height*width;
            for(int i=0; i<max_i; ++i)
                if( bitmap[i] && getWeightByDis(dismap[i], offset, sep) )
                    ++dirty;
            int error = dirty;
            printf("INFO: step %d. angle=%.2f, offset=%.2f, sep=%.2f, error=%d\n", step, arg/M_PI*180, offset, sep, error);
            if( error < best_error ){
                best_error = error;
                best_arg = arg;
                best_offset = offset;
                best_sep = sep;
                printf("INFO: better_angle=%.2f, better_offset=%.2f, better_sep=%.2f, error=%d\n", arg/M_PI*180, offset, sep, error);
            }
        }
    }
    free(dismap);
    data->error = best_error;
    data->arg = best_arg;
    data->offset = best_offset;
    data->sep = best_sep;
    return NULL;
}

int main(int argc, char*argv[]){
    if( argc!=2 ){
        printf("%s: photo_file\n", argv[0]);
        return 1;
    }

    FILE *in_f = fopen(argv[1], "rb");
    if( !in_f ){
        printf("ERROR: open file %s failed: %s\n", argv[1], strerror(errno));
        return 1;
    }

    gdImagePtr in_img;
    in_img = gdImageCreateFromJpeg(in_f);
    if( !in_img ){
        printf("\nERROR: read jpeg format from file %s failed\n", argv[1]);
        return 1;
    }
    fclose(in_f);
    in_f = NULL;

    int width = in_img->sx;
    int height = in_img->sy;
    printf("INFO: image size %dx%d\n", width, height);

    char * bitmap = (char*) malloc(width*height*sizeof(char));
    {
        int i = 0;
        for(int y=0; y<height; ++y)
            for(int x=0; x<width; ++x){
                if( THRESHOLD(gdImageGetPixel(in_img, x, y)) )
                    bitmap[i] = 1;
                else
                    bitmap[i] = 0;
                ++i;
            }
    }

    pthread_t thread[USE_THREAD];
    struct try_data_t try_data[USE_THREAD];
    for(int thread_i=0; thread_i<USE_THREAD; ++thread_i){
        try_data[thread_i].width = width;
        try_data[thread_i].height = height;
        try_data[thread_i].bitmap = bitmap;
        pthread_create(&thread[thread_i], NULL, try_core, &try_data[thread_i]);
    }
    int best_error = height * width;
    double best_arg, best_offset, best_sep;
    for(int thread_i=0; thread_i<USE_THREAD; ++thread_i){
        pthread_join(thread[thread_i], NULL);
        if( try_data[thread_i].error < best_error ){
            best_error = try_data[thread_i].error;
            best_arg = try_data[thread_i].arg;
            best_offset = try_data[thread_i].offset;
            best_sep = try_data[thread_i].sep;
        }
    }

    double diag = sqrt( width*width + height*height );
    double darg = atan2(height, width);

    int rotate_angle = best_arg >= 0 ? (int)( best_arg / M_PI * 180 + .5 ) : (int)( best_arg / M_PI * 180 + 360 + .5 );
    double min_y = best_arg >= 0. ? 0. : width * sin(best_arg);
    double min_x = best_arg <= 0. ? 0. : -height * sin(best_arg);
    double max_x = best_arg >= 0. ? width * cos(best_arg) : diag * cos(darg+best_arg);
    double max_y = best_arg <= 0. ? height * cos(best_arg) : diag * sin(darg+best_arg);
    printf("INFO: rotated boundary - min_x=%.2f, min_y=%.2f, max_x=%.2f, max_y=%.2f\n", min_x, min_y, max_x, max_y);

    gdImagePtr out_img = gdImageCreateTrueColor((int)(max_x-min_x+1), (int)(max_y-min_y+1));
    if( !out_img ){
        printf("\nERROR: create output image fail\n");
        return 1;
    }
    gdImageCopyRotated(out_img, in_img, (max_x-min_x+1)/2, (max_y-min_y+1)/2, 0, 0, width, height, rotate_angle);

    gdImagePtr strip_img = gdImageCreateTrueColor((int)(max_x-min_x+1), (int)(best_sep * 1.6 + .5));
    double cut_cursor = best_arg <= .0 ? best_offset : best_offset + width * sin(best_arg);
    while( cut_cursor >= 0. )
        cut_cursor -= best_sep;
    cut_cursor += best_sep;
    int strip_i = 0;
    while( cut_cursor < max_y-min_y+1 ){
        int src_y, dst_y, h;
        dst_y = 0;
        src_y = (int)(cut_cursor - 0.3 * best_sep + .5);
        h = (int)(best_sep * 1.6 + .5);
        if( src_y < 0 ){
            dst_y -= src_y;
            h += src_y;
            src_y = 0;
        }
        if( src_y + h > (int)(max_y-min_y+1) )
            h -= src_y - (int)(max_y-min_y+1);
        gdImageCopy(strip_img, out_img, 0, dst_y, 0, src_y, (int)(max_x-min_x+1), h);
        char strip_filename[1024];
        ++strip_i;
        snprintf(strip_filename, 1024, "cut_strip_%03d.jpg", strip_i);
        FILE *strip_f = fopen(strip_filename, "wb");
        gdImageJpeg(strip_img, strip_f, 95);
        fclose(strip_f);
        cut_cursor += best_sep;
    }

    // create debug image
    {
        int i = 0;
        for(int y=0; y<height; ++y)
            for(int x=0; x<width; ++x){
                if( getWeight(x, y, best_arg, best_offset, best_sep) )
                    if( bitmap[i] )
                        gdImageSetPixel(in_img, x, y, 0xFF0000);
                    else
                        gdImageSetPixel(in_img, x, y, 0xFF);
                else
                    if( bitmap[i] )
                        gdImageSetPixel(in_img, x, y, 0xFFFF00);
                    else
                        gdImageSetPixel(in_img, x, y, 0);
                ++i;
            }
        gdImageCopyRotated(out_img, in_img, (max_x-min_x+1)/2, (max_y-min_y+1)/2, 0, 0, width, height, rotate_angle);
        FILE *out_f = fopen("cut_line_output.jpg", "wb");
        gdImageJpeg(out_img, out_f, 95);
        fclose(out_f);
    }

    gdImageDestroy(strip_img);
    gdImageDestroy(out_img);
    gdImageDestroy(in_img);
    return 0;
}
