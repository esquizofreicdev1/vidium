
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mysql.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/ml.hpp>
#include <iostream>
using namespace std;
using namespace cv;
using namespace cv::ml;
int videoStreamIndex = 0;
#define FORMATO AV_PIX_FMT_RGB24

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <pthread.h>
}

    AVFormatContext  * ofmt_ctx = NULL;
    AVOutputFormat *ofmt = NULL;
    AVCodecContext * ofcodec_ctx;
    AVCodec * ocodec;
    AVStream *out_stream;
    AVFrame * pFrameYUV;
#define BACK_LOG 55
#define DEBUG(x) printf("%s%s",x,"\n")
#define ANDROID_OS "android"
#define IPHONE_OS "iphone"
#define IPAD_OS "ipad"
#define BLACKBERRY_OS "blackberry"

struct SwrContext *swrCtx = NULL;
AVFrame wanted_frame; 
static AVFrame * frame = av_frame_alloc();
static int videoStream = -1;
static pthread_mutex_t  * p;
 static bool ptr_op = false;
 static bool objectFilter = false;
static int counts = 0;
static AVFormatContext *pFormatCtx = avformat_alloc_context();
static AVCodecContext *pCodecCtx  = NULL;
static AVCodecContext *pCodecAudioCtx = NULL;


	//guarda o codec do v�deo
static AVCodec *pCodec = NULL;

//guarda o codec do audio
	static AVCodec *pAudioCodec = NULL;

	//estrutura que guarda o frame RGB
static AVFrame *pFrameRGB = av_frame_alloc();

	//buffer para leitura dos frames
static uint8_t *buffer = NULL;
static AVFrame * pFrame = av_frame_alloc();
static AVFrame * result_ptr = av_frame_alloc();
static AVFrame *dst  = av_frame_alloc();
static AVFrame* ptrYuv = av_frame_alloc();
static struct SwsContext *sws_ctx;
static cv::Mat ptr;


#define MAX_NUMBER_THREAD 500

int c = 0;

int elapsed_seconds;


typedef struct FFmpegConverter
{
    char * output;
    char  * input;
    FILE * fp;
    char  *dest;
    char *operating_system;
}FFmpegConverter;

typedef struct TimeElapsed
{
    char *buf_time;
}TimeElapsed;

TimeElapsed * copy_time;

TimeElapsed * timep;

vector<Point> track;

HOGDescriptor hog;

FFmpegConverter * ffmpegTranscoder;

typedef struct sock_server
{
    struct sockaddr_in server_st;
    int server_socket;
    int port;
}sock_server;

void init_transcoder()
{
    ffmpegTranscoder   = (FFmpegConverter* ) calloc ( 1, sizeof ( FFmpegConverter ) ) ;
}

void init_time()
{
     timep =  (TimeElapsed *  )calloc ( 1, sizeof (TimeElapsed));
}

int length(char *str)
{
    int characterCount = 0;

    while (str[characterCount] != '\0')
    {
        characterCount++;
    }
    return characterCount;
}
int buscarCadena(char * theString, char * searched) {
  int position = -1;
  char * result = NULL;

  if (theString && searched) {
    result = strstr(theString, searched);
    if (result) {
      position = result - theString;
    }
  }

  return position;
}
/*Starts looking for substring parameter in string from the passed start index*/
int indexOf(char *string, char *subString, int start)
{
    int flag;
    int indexOf = -1;
    int loopEnd = (int)(length(string)-length(subString));

    for (int i = start; i <= loopEnd; i++)
    {
        flag = 1;
        for (int index = 0; index < length(subString); index++)
        {
            if (string[i] != subString[index])
            {
                flag = 0;
                break;
            }
            else
            {
                i++;
            }
        }
        if (flag == 1)
        {
            indexOf = (int)(i - length(subString));
            break;
        }
    }

    return indexOf;
}

/*Helper function that returns the count of a particular delimiter in the string passed*/
int countOf(char *string, char *searchString)
{
    int index;
    int delimiterTally=0;
    int start = 0;

    while (1)
    {
        index = indexOf(string, searchString, start);
        if (index != -1)
        {
            delimiterTally ++;
        }
        else
        {
            break;
        }
        start = index + 1;
    }

    return delimiterTally;
}

/*Helper function that returns the substring*/
char* subString(char* string, int start, int length)
{
    char *a = (char*) malloc (length+1);
    int index;

    for (index=0; index<length; index++)
    {
        a[index] = string[index+start];
    }

    a[index] = '\0';

    return a;
}

/*Main driver function*/
char *string_replace(char *string, char *replaceFor, char *replaceWith)
{
    if (string == NULL || replaceFor == NULL || replaceWith == NULL)
    {
        return NULL;
    }

    int index;
    int subStringCount = countOf(string, replaceFor);


    /*One option to free the allocated memory here is for the caller to handle it.

     Another approach is to have the buffer as a method input parameter to make our intentions of who's

     going to handle the freeing part very clear
     */
    char *replacedString = (char*)malloc(length(string) - length(replaceFor) * subStringCount + length(replaceWith) * subStringCount);
    int start = 0, prevCount = 0, count = 0;

    while (1)
    {
        index = indexOf(string, replaceFor, start);
        if (index != -1)
        {
            for (int i = 0; i < index - prevCount; i++)
            {
                replacedString[count] = string[i+prevCount];
                count++;
            }
            for (int i = 0; i < length(replaceWith); i++)
            {
                replacedString[count] = replaceWith[i];
                count++;
            }
            prevCount = (int)(index + length(replaceFor));
        }
        else
        {
            break;
        }
        start = (int)(index + length(replaceFor));
    }

    char *temp = subString(string, prevCount, (int)length(string)-prevCount);

    for (int i = 0; i < length(temp); i++)
    {
        replacedString[count] = temp[i];
        count++;
    }

    replacedString[count] = '\0'; /*Terminating character*/

    return replacedString;
}

#define BUFFER_SIZE 1024

typedef struct sock_client
{
    struct sockaddr_in cliente_st;
    int client_socket;
}sock_client;



sock_server * server;
sock_client * cliente;
#define SQL_CONNECTED 1
#define SQL_FAILED 0

static MYSQL *  ret;
static int return_code = 0;

AVFrame *  convertMatToFrame(cv::Mat *   ptrResultBuffer)
{


          int numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
          uint8_t * frame2_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
          memcpy(frame2_buffer,(uint8_t*)ptrResultBuffer->data,numBytes);
          cv::Size frameSize = ptrResultBuffer->size();
                   dst->width = pCodecCtx->width;
                   dst->height  = pCodecCtx->height;
                   dst->format = AV_PIX_FMT_BGR24;


          avpicture_fill((AVPicture*)dst, frame2_buffer, AV_PIX_FMT_BGR24, pCodecCtx->width,pCodecCtx->height);

         avpicture_fill((AVPicture*)ptrYuv,(uint8_t*)dst->data[0],AV_PIX_FMT_YUV420P,pCodecCtx->width,pCodecCtx->height);
           struct SwsContext * swsContext;
             swsContext = sws_getContext(pCodecCtx->width,pCodecCtx->height,
                     AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height,
                     AV_PIX_FMT_YUV420P,
                     SWS_BICUBIC,
                     NULL,
                     NULL,
                     NULL);

   sws_scale(swsContext, dst->data, dst->linesize, 0, pCodecCtx->height,ptrYuv->data, ptrYuv->linesize);
return (ptrYuv);
}


void iniciarParametros()
{
                  sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                  pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                  AV_PIX_FMT_YUV420P,
                  SWS_FAST_BILINEAR,
                  NULL,
                  NULL,
                  NULL);
}

cv::Mat convertFrameToMat()
{

    result_ptr = av_frame_alloc();

    int size_buf = avpicture_get_size(AV_PIX_FMT_BGR24,pCodecCtx->width,pCodecCtx->height);
    uint8_t * output =  (uint8_t * ) malloc(size_buf);
    avpicture_fill((AVPicture*)result_ptr,output,AV_PIX_FMT_BGR24,pCodecCtx->width,pCodecCtx->height);
    struct SwsContext * swsContext;
    swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height,
            pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
            AV_PIX_FMT_BGR24,
            SWS_FAST_BILINEAR,
            NULL,
            NULL,
            NULL);

    sws_scale(swsContext,pFrame->data, pFrame->linesize, 0, pCodecCtx->height,result_ptr->data, result_ptr->linesize);
    cv::Mat image(pCodecCtx->height, pCodecCtx->width, CV_8UC3, output, result_ptr->linesize[0]);
    return (image);
}

cv::Mat applyDetectionFilter(cv::Mat ptrResult)
{
    Mat current_frame = ptrResult;
    Mat img  = current_frame.clone();
    //Redimensionamos la imagen.
        vector<Rect> found;
        vector<double> weights;
        hog.detectMultiScale(img, found, weights);

        /// draw detections and store location
        for( size_t i = 0; i < found.size(); i++ )
        {
        
            rectangle(img, found[i], cv::Scalar(0,0,255), 3);
            stringstream temp;
            temp << weights[i];
            putText(img, temp.str(),Point(found[i].x,found[i].y+50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0,0,255));
            track.push_back(Point(found[i].x+found[i].width/2,found[i].y+found[i].height/2));
        }

        /// plot the track so far
        for(size_t i = 1; i < track.size(); i++){
            line(img, track[i-1], track[i], Scalar(255,255,0), 2);
        }
        return (img);

}

bool abrirStream(char *path)
{
                bool resultPointer = true;

		//init ffmpeg
		av_register_all();
                avcodec_register_all();
                avformat_network_init();
        
                pFormatCtx = avformat_alloc_context();


		//open video
		int res = avformat_open_input(&pFormatCtx, path, NULL,NULL);



		if (res < 0 ){
                        DEBUG("Error al abrir el archivo..");
                        resultPointer = false;
		}

		else
                {
                    //get video info
		res = avformat_find_stream_info(pFormatCtx, NULL);
		if (res < 0) {
                        DEBUG("No se pudo obtener la info");
                        resultPointer = false;
		}

                
                resultPointer = true;
}
                return ( resultPointer ) ;


                   
}
void encodeMJPEG() {
    int copyBytes;
    AVPacket packet;
    AVFrame * r = av_frame_alloc();
    AVPacket outpkg;
    av_new_packet(&outpkg, ofcodec_ctx->width * ofcodec_ctx->height * 3);
    av_init_packet(&packet);
    int got_picture = 0;
    int flag = 1;

    int frame_count = 0;
   
    hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
	while (av_read_frame(pFormatCtx, &packet) >= 0 && flag){
                packet.dts = av_rescale_q_rnd(packet.dts,
                pFormatCtx->streams[videoStreamIndex]->time_base,
                pFormatCtx->streams[videoStreamIndex]->codec->time_base,
                (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                flag = 1;
  packet.pts = av_rescale_q_rnd(packet.pts,
                pFormatCtx->streams[videoStreamIndex]->time_base,
                pFormatCtx->streams[videoStreamIndex]->codec->time_base,
                (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

  packet.duration = av_rescale_q_rnd(packet.duration,
                pFormatCtx->streams[videoStreamIndex]->time_base,
                pFormatCtx->streams[videoStreamIndex]->codec->time_base,
                (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		if (packet.stream_index == videoStreamIndex) {
                    DEBUG("Es un frame de video index == videoStream TRUE!!");
                    
                    int res = avcodec_decode_video2(pCodecCtx,pFrame,&copyBytes,&packet);
                    if ( res < 0 )
                    {
                        DEBUG("Error decoding video data..");
                    }
                    if ( copyBytes )
                    {

                        cv::Mat resultBuffer = convertFrameToMat();
                        cv::Mat filtered = applyDetectionFilter(resultBuffer);
                        AVFrame * resultFrame = convertMatToFrame(&filtered);
                       frame_count++;
                       resultFrame->pts = frame_count;
                       std::cout << resultFrame->pts << std::endl;
                       int ret = avcodec_encode_video2(ofcodec_ctx, &outpkg,
                            resultFrame, &got_picture);


                      if ( got_picture == 1 && flag == 1) 
                      {
                             av_write_frame(ofmt_ctx, &outpkg);
                             av_write_trailer(ofmt_ctx);
                      }
                      flag = 0;
                    }





		}                 

            


	}



}

void encoderSetup(char  * input_file,char  *dest)
{
    pFormatCtx->probesize = 20000000;
    pFormatCtx->max_analyze_duration = 2000;
    bool result_conn = abrirStream(input_file);
    if ( result_conn ) 
    {
       DEBUG("Stream opened successfully encoding to MJPEG..");
    videoStreamIndex = av_find_best_stream(
    pFormatCtx,        // The media stream
    AVMEDIA_TYPE_VIDEO,   // The type of stream we are looking for - audio for example
    -1,                   // Desired stream number, -1 for any
    -1,                   // Number of related stream, -1 for none
    &pCodec,          // Gets the codec associated with the stream, can be NULL
    0                     // Flags - not used currently
);

    AVStream* videoFlujo = pFormatCtx->streams[videoStreamIndex];
    pCodecCtx = videoFlujo->codec;
    pCodecCtx->codec = pCodec;


  if (avcodec_open2(pCodecCtx, pCodecCtx->codec, NULL) < 0 )
    {
              DEBUG("Error opening codec..");
    }
       av_dump_format(pFormatCtx, 0, input_file, 0);
            ofmt_ctx = avformat_alloc_context();
            ofmt = av_guess_format("mjpeg", NULL, NULL);
            ofmt_ctx->oformat = ofmt;
            out_stream = avformat_new_stream(ofmt_ctx, NULL);
            if ( !out_stream ) DEBUG("Error creating output stream..");
    ofcodec_ctx = out_stream->codec;
    ofcodec_ctx->codec_id = ofmt->video_codec;
    ofcodec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    ofcodec_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    ofcodec_ctx->width = pCodecCtx->width;
    ofcodec_ctx->height = pCodecCtx->height;
    ofcodec_ctx->time_base.den = 30;
    ofcodec_ctx->time_base.num = 1;
   ofcodec_ctx->bit_rate = 1000000;
       AVDictionary * opts = NULL;
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    out_stream->codec->codec_tag = 0;
    ocodec = avcodec_find_encoder(ofcodec_ctx->codec_id);
    if (!ocodec) {
        printf("find encoder err\n");
    }
    if (avcodec_open2(ofcodec_ctx, ocodec, NULL) < 0) {
        printf("open encoder err\n");
    }

    av_dump_format(ofmt_ctx, 0, dest, 1);
    if (!(ofmt->flags & AVFMT_NOFILE)) {
                        int resultado  = avio_open(&ofmt_ctx->pb, dest,
                        AVIO_FLAG_WRITE);
                        if (resultado  < 0) {
                            printf("could not open output url '%s'\n",
                                    dest);

                        }
                    }

          int result = avformat_write_header(ofmt_ctx, NULL);
  
}

}








int main(int argc,char ** argv)
{
  char *argumento = (char  * )strdup(argv[1]); 
  char  *argumentodos =  (char  * )strdup(argv[2]);
  encoderSetup(argumento,argumentodos);
  encodeMJPEG();
  DEBUG("Output file with the OpenCV detection successfully..");
  return 0;
}

