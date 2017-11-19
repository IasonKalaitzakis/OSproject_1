
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



void initialize_FCB(FCB* fcb){

	fcb->refcount = 1;  			/**< @brief Reference counter. */
  	fcb->streamobj = ;			/**< @brief The stream object (e.g., a device) */
  	file_ops* streamfunc;		/**< @brief The stream implementation methods */
  	rlnode freelist_node;

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

int sys_Pipe(pipe_t* pipe)
{	

	Fid_t arrayOfFIDs[2];
	FCB* arrayOfFCBPointers[2];
	FCB_reserve(2, arrayOfFIDs, arrayOfFCBPointers);

	pipe->read = arrayOfFIDs[0];
	pipe->write = arrayOfFIDs[1];

	readerFCB = arrayOfFCBPointers[0];
	writerFCB = arrayOfFCBPointers[1];

}

