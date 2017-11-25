
#include "tinyos.h"

#define BUFFER_SIZE 10000

typedef struct pipe_control_block {

	char buffer[BUFFER_SIZE];
	char* writerHead;
	char* readerHead;

	CondVar hasSpace;
	CondVar hasData;

	FCB* writePtr;
	FCB* readerPtr;
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

int pipe_write(){


}

int pipe_read(){



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

	pipecb->writerHead = pipecb->buffer[0];
	pipecb->readerHead = pipecb->buffer[0];

	return;

}
