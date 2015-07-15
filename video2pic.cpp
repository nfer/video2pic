
#include <stdio.h>
#include "opencv2/highgui/highgui_c.h"

#ifdef __cplusplus
extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
};
#endif

int bmp_write(unsigned char *image, int xsize, int ysize, int bpp, char *filename) {
    unsigned char header[54] = {
      0x42, 0x4d, 0, 0, 0, 0, 0, 0, 0, 0,
        54, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0
    };
    
    long file_size = (long)xsize * (long)ysize *bpp/8 + 54;
    header[2] = (unsigned char)(file_size &0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;
    
    long width = xsize;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) &0x000000ff;
    header[20] = (width >> 16) &0x000000ff;
    header[21] = (width >> 24) &0x000000ff;
    
    long height = ysize;
    header[22] = height &0x000000ff;
    header[23] = (height >> 8) &0x000000ff;
    header[24] = (height >> 16) &0x000000ff;
    header[25] = (height >> 24) &0x000000ff;
    
    FILE *fp = fopen(filename, "wb");
    if (NULL == fp) 
      return -1;
      
    fwrite(header, sizeof(unsigned char), 54, fp);
    fwrite(image, sizeof(unsigned char), (size_t)(long)xsize * ysize *bpp/8, fp);
    
    fclose(fp);
    return 0;
}

int bmp2jpg(const char * bmp, const char * jpg)
{
	IplImage *src=cvLoadImage(bmp);
	if(!src) {
		printf ("can not load the image %s \n", bmp);
		return -1;
	}

	printf ("processing...\n");
	cvSaveImage(jpg,src);
	cvReleaseImage(&src);
	return 0;
}

void SaveAsBMP (AVFrame *pFrameRGB, int width, int height, int index, int bpp)
{
	printf ("SaveAsBMP %d,%d,%d,%d!\n", width, height, index, bpp);

	char bmp[255] = {'\0'}, jpg[255] = {'\0'};
	snprintf(bmp, sizeof(bmp), "./output/bmp_%d.bmp", index);
	snprintf(jpg, sizeof(jpg), "./output/jpg_%d.jpg", index);
	printf ("bmp %s!\n", bmp);
	printf ("jpg %s!\n", jpg);
	bmp_write(pFrameRGB->data[0], width, height, bpp, bmp);
	bmp2jpg(bmp, jpg);
	remove(bmp);
	printf ("write file OK!\n");
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("Please input %s FileName.\n", argv[0]);
		return -1;
	}
	char * filepath=argv[1];
	printf("file name is %s\n", filepath);

	int start = -1, end = -1;
	if (argc > 3) {
		start = atoi(argv[2]);
		end = atoi(argv[3]);

		if (start < 0 || end < 0 || end < start) {
			printf("Please input right start(%s) and end(%s) index.\n", argv[2], argv[3]);
			return -1;
		}
	}
	printf("start at %d, end at %d\n", start, end);

	av_register_all();
	
	AVFormatContext	*pFormatCtx = avformat_alloc_context();
	if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if(avformat_find_stream_info(pFormatCtx,NULL)<0){
		printf("Couldn't find stream information.\n");
		return -1;
	}

	int videoindex = -1;
	for(int i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			break;
		}
	}

	if(videoindex == -1){
		printf("Didn't find a video stream.\n");
		return -1;
	}

	AVCodecContext *pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec == NULL){
		printf("Codec not found.\n");
		return -1;
	}

	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}

	//Output Information-----------------------------
	printf("------------- File Information ------------------\n");
	av_dump_format(pFormatCtx,0,filepath,0);
	printf("-------------------------------------------------\n");

	//------------------------------
	AVFrame	*pFrame = avcodec_alloc_frame();
	AVFrame	*pFrameRGB = avcodec_alloc_frame();
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	int pictureSize = avpicture_get_size (PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
	uint8_t *buf = (uint8_t*)av_malloc(pictureSize);
	avpicture_fill ( (AVPicture *)pFrameRGB, buf, PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
	int ret, got_picture;
	int count = 0;
	while(av_read_frame(pFormatCtx, packet)>=0){
		if(packet->stream_index == videoindex){
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if(ret < 0){
				printf("Decode Error.\n");
				return -1;
			}

			if(got_picture){
				count++;
				if (count <= start)
					continue;

				if (end != -1 && count > end+1)
					break;

				pFrame->data[0] += pFrame->linesize[0] * (pCodecCtx->height - 1);
				pFrame->linesize[0] *= -1;
				pFrame->data[1] += pFrame->linesize[1] * (pCodecCtx->height / 2 - 1);
				pFrame->linesize[1] *= -1;
				pFrame->data[2] += pFrame->linesize[2] * (pCodecCtx->height / 2 - 1);
				pFrame->linesize[2] *= -1;

				printf("get one picture. pts:%ld, pkt_pts:%ld, pkt_dts:%ld\n", pFrame->pts, pFrame->pkt_pts, pFrame->pkt_dts);
				sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,
					0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
				SaveAsBMP (pFrameRGB, pCodecCtx->width, pCodecCtx->height, count, 24);
			}
		}
		av_free_packet(packet);
	}
	sws_freeContext(img_convert_ctx);
	av_free(buf);
	av_free(pFrameRGB);
	av_free(pFrame);

	printf("picture count is %d.\n", count);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}