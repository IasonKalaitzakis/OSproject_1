
#include "tinyos.h"

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
}PipeCB;



void initialize_FCB(FCB* fcb, PipeCB* pipecb, int readerFlag){

	fcb->refcount = 1;  			/**< @brief Reference counter. */
  	fcb->streamobj = pipecb;			/**< @brief The stream object (e.g., a device) */

	if (readerFlag == 0){
		fcb->streamfunc = writer_ops;
	}
	else {
		fcb->streamfunc = reader_ops;
	}
  	rlnode_init(& fcb->freelist_node, fcb);


} 

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


int pipe_reader_write(){return -1;}
int pipe_writer_read(){return -1;}
int pipe_open(){return -1;}

int pipe_write(void* pipe, char *buf, unsigned int size){

	PipeCB* pipecb = (PipeCB*) pipe;
	int	bytesWritten = 0;

	if(pipecb->readerHead == pipecb->writerHead && pipecb->bufferChars ==BUFFER_SIZE){
		kernel_wait(&pipecb->hasSpace, SCHED_PIPE);                                                             
	}

	


}

int pipe_read(void* pipe, char *buf, unsigned int size){

	PipeCB* pipecb = (PipeCB*) pipe;
	int	bytesCopied = 0;


	if(pipecb->readerHead == pipecb->writerHead && pipecb->bufferChars ==0){
		kernel_wait(&pipecb->hasData, SCHED_PIPE);                                                             
	}

	if (pipecb->bufferChars<0){
		fprintf(stderr, "Buffer chars fell below 0");
	}

	//Buffer is not linear (loops around 0)
	if(pipecb->readerHead>pipecb->writerHead){

		//Reader index crosses 0 and starts over
		if(pipecb->readerHead+size>BUFFER_SIZE-1){

			//There are enough chars to return size number of chars
			//if ((BUFFER_SIZE - pipecb->readerHead) + pipecb->writerHead >= size){
			if (pipecb->bufferChars>=size){

				bytesCopied = BUFFER_SIZE - pipecb->readerHead;
				memmove(buf, &pipecb->buffer[readerHead], bytesCopied);
				pipecb->readerHead = 0;
				bytesCopied = size - bytesCopied;
				memmove(buf+bytesCopied, &pipecb->buffer[readerHead], bytesCopied);
				pipecb->readerHead = bytesCopied;
				pipecb->bufferChars = pipecb->bufferChars - size;
				bytesCopied = size;
				
			}
			//Size is bigger than available chars
			else{

				bytesCopied = BUFFER_SIZE - pipecb->readerHead;
				memmove(buf, &pipecb->buffer[readerHead], bytesCopied);
				pipecb->readerHead = 0;
				memmove(buf+bytesCopied, &pipecb->buffer[readerHead], pipecb->writerHead);
				pipecb->readerHead = pipecb->writerHead;
				bytesCopied = bytesCopied + pipecb->writerHead;
				pipecb->bufferChars = pipecb->bufferChars - bytesCopied;
				
			}
		}

		//Reader index doesnt cross 0 (also means that the size will definitely be enough)
		else {

			bytesCopied = size;
			memmove(buf, &pipecb->buffer[readerHead], bytesCopied);
			pipecb->readerHead = pipecb->readerHead + bytesCopied;
			pipecb->bufferChars = pipecb->bufferChars - bytesCopied;
			
		}
	}

	//Buffer doesnt loop around 0
	else {

		//Size is bigger than available chars
		if(pipecb->readerHead + size > pipecb->writerHead){

			bytesCopied = pipecb->writerHead-pipecb->readerHead;

			memmove(buf, &pipecb->buffer[readerHead], bytesCopied);
			pipecb->readerHead = pipecb->writerHead;
			pipecb->bufferChars = pipecb->bufferChars - bytesCopied;
			
		}

		//Buffer has enough chars for the size
		else{

			bytesCopied = size;
			memmove(buf, &pipecb->buffer[readerHead], bytesCopied);
			pipecb->bufferChars = pipecb->bufferChars - bytesCopied;
			pipecb->readerHead = pipecb->readerHead + bytesCopied;
			
		} 
	}

	kernel_broadcast(&pipecb->hasSpace);
	
	return bytesCopied;

}

int pipe_writer_close(){



}


int pipe_reader_close(){




}

int sys_Pipe(pipe_t* pipe)
{	

	PipeCB* pipecb = (PipeCB*)xmalloc(sizeof(PipeCB));

	Fid_t arrayOfFIDs[2];
	FCB* arrayOfFCBPointers[2];
	FCB_reserve(2, arrayOfFIDs, arrayOfFCBPointers);

	pipe->read = arrayOfFIDs[0];
	pipe->write = arrayOfFIDs[1];

	if (pipe->read ==NULL || pipe_write==NULL){
		return -1;
	}

	readerFCB = arrayOfFCBPointers[0];
	writerFCB = arrayOfFCBPointers[1];

	createPipe(writerFCB,readerFCB, pipecb);

	initialize_FCB(readerFCB,pipecb,1);
	initialize_FCB(writerFCB,pipecb,0);


	return 0;

}


void createPipe(FCB* writerFCB, FCB* readerFCB, PipeCB* pipecb){

	pipecb->writePtr = writerFCB;
	pipecb->readerPtr = readerFCB;

	pipecb->hasData = COND_INIT;
	pipecb->hasSpace = COND_INIT;

	pipecb->writerHead = 0;
	pipecb->readerHead = 0;
	bufferChars = 0;

	return;

}
