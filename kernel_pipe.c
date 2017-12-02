
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_dev.h"
#include "kernel_cc.h"




#define BUFFER_SIZE 10000

typedef struct pipe_control_block {

	char buffer[BUFFER_SIZE];
	int writerHead;
	int readerHead;

	CondVar hasSpace;
	CondVar hasData;

	FCB* writePtr;
	FCB* readerPtr;

	int bufferChars;

	int flagNoWriters;
	int flagNoReaders;

	//Mutex m;

}PipeCB;


int pipe_write(void* pipe,const char *buf, unsigned int size);
int pipe_read(void* pipe, char *buf, unsigned int size);
int pipe_reader_write(void* pipe, const char *buf, unsigned int size);
int pipe_writer_read(void* pipe, char *buf, unsigned int size);
void* pipe_open(uint minor);
int pipe_reader_close(void* pipe);
int pipe_writer_close(void* pipe);
void createPipe(FCB* writerFCB, FCB* readerFCB, PipeCB* pipecb);




static file_ops reader_ops = {

	.Open = pipe_open,
  	.Read = pipe_read,
  	.Write = pipe_reader_write,
  	.Close = pipe_reader_close
};

static file_ops writer_ops = {

	.Open = pipe_open,
  	.Read = pipe_writer_read,
  	.Write = pipe_write,
  	.Close = pipe_writer_close
};

void initialize_FCB(FCB* fcb, PipeCB* pipecb, int readerFlag){

	//fcb->refcount = 1;  			/**< @brief Reference counter. */
  	fcb->streamobj = pipecb;			/**< @brief The stream object (e.g., a device) */

	if (readerFlag == 0){
		fcb->streamfunc = &writer_ops;
	}
	else {
		fcb->streamfunc = &reader_ops;
	}
  	rlnode_init(& fcb->freelist_node, fcb);


} 


int pipe_reader_write(void* pipe,const char *buf, unsigned int size){return -1;}
int pipe_writer_read(void* pipe, char *buf, unsigned int size){return -1;}
void* pipe_open(uint minor){return NULL;
}

int pipe_write(void* pipe,const char *buf, unsigned int size){

	PipeCB* pipecb = (PipeCB*) pipe;
	int	bytesWritten = 0;


	if(pipecb->flagNoReaders == 1){return -1;}


	while(pipecb->readerHead == pipecb->writerHead && pipecb->bufferChars ==BUFFER_SIZE){
		kernel_wait(&pipecb->hasSpace, SCHED_PIPE);                                                             
	}



	//Reader index is behind of the writer index
	if(pipecb->readerHead<=pipecb->writerHead){

		//Writer index crosses 0 and starts over
		if(pipecb->writerHead+size>BUFFER_SIZE-1){

			//There are enough free array spots to write all bytes(size) of the buffer
			//if ((BUFFER_SIZE - pipecb->readerHead) + pipecb->writerHead >= size){
			if (BUFFER_SIZE - pipecb->bufferChars>=size){	

				bytesWritten = BUFFER_SIZE - pipecb->writerHead;
				memmove(&pipecb->buffer[pipecb->writerHead],buf, bytesWritten);
				pipecb->writerHead = 0;
				bytesWritten = size - bytesWritten;
				memmove(&pipecb->buffer[pipecb->writerHead],buf+bytesWritten,bytesWritten);
				pipecb->writerHead = bytesWritten;
				pipecb->bufferChars = pipecb->bufferChars + size;
				bytesWritten = size;

			}
			//Size is bigger than free array slots
			else{
				
				bytesWritten = BUFFER_SIZE - pipecb->writerHead;
				memmove(&pipecb->buffer[pipecb->writerHead], buf, bytesWritten);
				pipecb->writerHead = 0;
				memmove(&pipecb->buffer[pipecb->writerHead], buf+bytesWritten , pipecb->readerHead);
				pipecb->writerHead = pipecb->readerHead;
				bytesWritten = bytesWritten + pipecb->writerHead;
				pipecb->bufferChars = pipecb->bufferChars + bytesWritten;
			}
		}
		//Writer index doesnt cross 0 (also means that the size will definitely be enough)
		else {

			bytesWritten = size;
			memmove(&pipecb->buffer[pipecb->writerHead],buf,bytesWritten);
			pipecb->writerHead = pipecb->writerHead + bytesWritten;
			pipecb->bufferChars = pipecb->bufferChars + bytesWritten;	
		}
	}

	else {

		//Size is bigger than available chars
		if(BUFFER_SIZE - pipecb->bufferChars <size){

			bytesWritten = pipecb->readerHead-pipecb->writerHead;
			memmove(&pipecb->buffer[pipecb->writerHead],buf, bytesWritten);
			pipecb->writerHead = pipecb->readerHead;
			pipecb->bufferChars = pipecb->bufferChars + bytesWritten;
		}

		//Buffer has enough chars for the size
		else{

			bytesWritten = size;
			memmove(&pipecb->buffer[pipecb->writerHead],buf, bytesWritten);
			pipecb->bufferChars = pipecb->bufferChars + bytesWritten;
			pipecb->writerHead = pipecb->writerHead + bytesWritten;
		} 
	}

 	kernel_broadcast(&pipecb->hasData);
	
	return bytesWritten;

}

int pipe_read(void* pipe, char *buf, unsigned int size){

	PipeCB* pipecb = (PipeCB*) pipe;
	int	bytesCopied = 0;

	if(pipecb->flagNoWriters == 1 && pipecb->readerHead == pipecb->writerHead && pipecb->bufferChars ==0){
		return 0;
	}


	while(pipecb->readerHead == pipecb->writerHead && pipecb->bufferChars ==0){
		kernel_wait(&pipecb->hasData, SCHED_PIPE);                                                             
	}

	if (pipecb->bufferChars<0){
		fprintf(stderr, "Buffer chars fell below 0");
	}

	//Reader index is ahead of writer index
	if(pipecb->readerHead>=pipecb->writerHead){

		//Reader index crosses 0 and starts over
		if(pipecb->readerHead+size>BUFFER_SIZE-1){

			//There are enough chars to return size number of chars
			//if ((BUFFER_SIZE - pipecb->readerHead) + pipecb->writerHead >= size){
			if (pipecb->bufferChars>=size){

				bytesCopied = BUFFER_SIZE - pipecb->readerHead;
				memmove(buf, &pipecb->buffer[pipecb->readerHead], bytesCopied);
				pipecb->readerHead = 0;
				bytesCopied = size - bytesCopied;
				memmove(buf+bytesCopied, &pipecb->buffer[pipecb->readerHead], bytesCopied);
				pipecb->readerHead = bytesCopied;
				pipecb->bufferChars = pipecb->bufferChars - size;
				bytesCopied = size;
				
			}
			//Size is bigger than available chars
			else{

				bytesCopied = BUFFER_SIZE - pipecb->readerHead;
				memmove(buf, &pipecb->buffer[pipecb->readerHead], bytesCopied);
				pipecb->readerHead = 0;
				memmove(buf+bytesCopied, &pipecb->buffer[pipecb->readerHead], pipecb->writerHead);
				pipecb->readerHead = pipecb->writerHead;
				bytesCopied = bytesCopied + pipecb->writerHead;
				pipecb->bufferChars = pipecb->bufferChars - bytesCopied;
				
			}
		}

		//Reader index doesnt cross 0 (also means that the size will definitely be enough)
		else {

			bytesCopied = size;
			memmove(buf, &pipecb->buffer[pipecb->readerHead], bytesCopied);
			pipecb->readerHead = pipecb->readerHead + bytesCopied;
			pipecb->bufferChars = pipecb->bufferChars - bytesCopied;
			
		}
	}

	//Buffer doesnt loop around 0
	else {

		//Size is bigger than available chars
		if(pipecb->readerHead + size > pipecb->writerHead){

			bytesCopied = pipecb->writerHead-pipecb->readerHead;

			memmove(buf, &pipecb->buffer[pipecb->readerHead], bytesCopied);
			pipecb->readerHead = pipecb->writerHead;
			pipecb->bufferChars = pipecb->bufferChars - bytesCopied;
			
		}

		//Buffer has enough chars for the size
		else{

			bytesCopied = size;
			memmove(buf, &pipecb->buffer[pipecb->readerHead], bytesCopied);
			pipecb->bufferChars = pipecb->bufferChars - bytesCopied;
			pipecb->readerHead = pipecb->readerHead + bytesCopied;

		} 
	}

	kernel_broadcast(&pipecb->hasSpace);
	
	return bytesCopied;

}

int pipe_writer_close(void* pipe){

	PipeCB* pipecb = (PipeCB*) pipe;
	kernel_broadcast(&pipecb->hasData);

	pipecb->flagNoWriters = 1;

	if(pipecb->flagNoWriters == 1 && pipecb->flagNoReaders == 1){
		free(pipecb);
	}

	return 0;


}


int pipe_reader_close(void* pipe){

	PipeCB* pipecb = (PipeCB*) pipe;

	//if (pipecb->flagNoWriters == 0){
	//	return -1;
	//}

	pipecb->flagNoReaders = 1;

	if(pipecb->flagNoWriters == 1 && pipecb->flagNoReaders == 1){
		free(pipecb);
	}

	return 0;

}

int sys_Pipe(pipe_t* pipe)
{	

	PipeCB* pipecb = (PipeCB*)xmalloc(sizeof(PipeCB));

	Fid_t arrayOfFIDs[2];
	FCB* arrayOfFCBPointers[2];
	if(FCB_reserve(2, arrayOfFIDs, arrayOfFCBPointers)==0){
		return -1;
	}

	pipe->read = arrayOfFIDs[0];
	pipe->write = arrayOfFIDs[1];

	pipecb->readerPtr = arrayOfFCBPointers[0];
	pipecb->writePtr = arrayOfFCBPointers[1];

	createPipe(pipecb->writePtr,pipecb->readerPtr, pipecb);

	initialize_FCB(pipecb->readerPtr,pipecb,1);
	initialize_FCB(pipecb->writePtr,pipecb,0);


	return 0;

}


void createPipe(FCB* writerFCB, FCB* readerFCB, PipeCB* pipecb){

	pipecb->writePtr = writerFCB;
	pipecb->readerPtr = readerFCB;

	pipecb->hasData = COND_INIT;
	pipecb->hasSpace = COND_INIT;

	pipecb->writerHead = 0;
	pipecb->readerHead = 0;
	pipecb->bufferChars = 0;
	pipecb->flagNoWriters = 0;
	pipecb->flagNoReaders = 0;

	//pipecb->m = MUTEX_INIT;
 

	return;

}
