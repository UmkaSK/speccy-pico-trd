#include <VFS.h>

#include <fcntl.h>
#include <zx_emu/z80.h>
#include "g_config.h"
#include "util_sd.h"

//#define FF_USE_LFN;
//extern uint8_t zx_color[];



//int fd =-1;
int last_error=0;
bool init_fs;


//работа с каталогами и файлами

bool init_filesystem(void){
    DIR d;
    int fd =-1;
  /*   strcpy(dirs[0],"0:");
    strcpy(dirs[1],"z80");
    strcpy(dirs[2],"a");
    strcpy(files[0],"..");
     */

    files[0][13]=1;
    //read_select_dir(1);
    strcpy(activefilename,"");
    fd=f_opendir(&d,dirs[0]);
    if (fd!=FR_OK) return false;    
    printf("Open SD-OK\n");
    return true;
}

const char* get_file_extension(const char *filename){
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

void sortfiles(char* nf_buf, int N_FILES){
    int inx=0;
    if (N_FILES==0) return;
    uint8_t tmp_buf[14];
    while(inx!=(N_FILES-1)){
        if (nf_buf[14*inx+13]>nf_buf[14*(inx+1)+13]){inx++; continue;}

        if ((nf_buf[14*inx+13]<nf_buf[14*(inx+1)+13])||(strcmp(nf_buf+14*inx,nf_buf+14*(inx+1))>0)){
            memcpy(tmp_buf,nf_buf+14*inx,14);
            memcpy(nf_buf+14*inx,nf_buf+14*(inx+1),14);
            memcpy(nf_buf+14*(inx+1),tmp_buf,14);
            if (inx) inx--;
            continue;
        }
        inx++;
    }
}

int get_files_from_dir(char *dir_name,char* nf_buf, int MAX_N_FILES){
    DIR d;
    int fd =-1;
    FILINFO dir_file_info;
    //"0:/z80"
    //fd = f_opendir(&d,dir_name);
    fd=f_opendir(&d,dir_name);
    if (fd!=FR_OK){init_fs=false;return 0;}

    //printf("\nFile FD: %d\n", fd);
    int inx=0;
    while (1){   
        
        fd=f_readdir(&d,&dir_file_info);
        if (fd!=FR_OK){init_fs=false;return 0;}
        if (strlen(dir_file_info.fname)==0) break;
        if(strlen(dir_file_info.fname)>13){
//            strncpy(nf_buf+14*inx,dir_file_info.altname,13) ;    
            strncpy(nf_buf+14*inx,dir_file_info.fname,13) ;
        } else {
            strncpy(nf_buf+14*inx,dir_file_info.fname,13) ;
        }
        if (dir_file_info.fattrib == AM_DIR) nf_buf[14*inx+13]=1; else nf_buf[14*inx+13]=0;
        inx++; 
        if (inx>=MAX_N_FILES) break;
        //if (dir_file_info.fname) break;
        //file_list[i] = dir_file_info.fname;
        //printf("[%s] %d\n",dir_file_info.fname,strlen(dir_file_info.fname));
    }
    f_closedir(&d);
    sortfiles(nf_buf,inx);
    return inx;
}

char* get_file_from_dir(char *dir_name,int inx_file){

    DIR d;
    int fd =-1;
    FILINFO dir_file_info;
    //static char filename[20];
    static char filename[256];//static char filename[200];
    //filename[0]=0;
    //filename[14]=0;    
    memset(filename, 0, 256);

    fd = f_opendir(&d,dir_name);
    if (fd!=FR_OK) return filename;
    //printf("\nFile FD: %d\n", fd);
    int inx=0;
    while (1){   
        fd = f_readdir(&d,&dir_file_info);
        if (fd!=FR_OK) return filename;
        if (strlen(dir_file_info.fname)==0) break;
        if (inx++==inx_file){
            //strncpy(filename,dir_file_info.fname,14) ;
            strncpy(filename,dir_file_info.fname,sizeof(dir_file_info.fname)) ;
            if (dir_file_info.fattrib == AM_DIR) strcat(filename,"*");
            break;
        }
        //if (dir_file_info.fname) break;
        //file_list[i] = dir_file_info.fname;
    }
    f_closedir(&d);
    //printf("[%s] %d\n",filename,strlen(filename));
    return filename;

}

char* get_lfn_from_dir(char *dir_name,char *part_name){

    DIR d;
    int fd =-1;
    FILINFO dir_file_info;
    //static char filename[20];
    static char filename[256];
    //filename[0]=0;
    //filename[14]=0;    
    memset(filename, 0, 200);

    fd = f_opendir(&d,dir_name);
    if (fd!=FR_OK) return filename;
    //printf("\nFile FD: %d\n", fd);
    int inx=0;
    while (1){   
        fd = f_readdir(&d,&dir_file_info);
        if (fd!=FR_OK) return filename;
        if (strlen(dir_file_info.fname)==0) break;
        //printf("[%s] %d> %d\n",dir_file_info.fname,strlen(dir_file_info.fname),strncmp(dir_file_info.fname,part_name,14));
        //printf("[%s] %s\n",dir_file_info.fname,dir_file_info.altname);
/*        if (strncmp(dir_file_info.altname,part_name,13)==0){
            strncpy(filename,dir_file_info.fname,sizeof(dir_file_info.fname)) ;
            //if (dir_file_info.fattrib == AM_DIR) strcat(filename,"*");
            break;
        }*/
        if (strncmp(dir_file_info.fname,part_name,13)==0){
            strncpy(filename,dir_file_info.fname,sizeof(dir_file_info.fname)) ;
            //if (dir_file_info.fattrib == AM_DIR) strcat(filename,"*");
            break;
        }

        //if (dir_file_info.fname) break;
        //file_list[i] = dir_file_info.fname;
        inx++;
    }
    f_closedir(&d);
    //printf(">[%s] %d\n",filename,strlen(filename));
    return filename;

}


int read_select_dir(int dir_index){
    dir_patch[0]=0;
    for(int i=0;i<=dir_index;i++){
      if (dir_index>=DIRS_DEPTH) break;
      
      strcat(dir_patch,dirs[i]);
      if (i!=dir_index) strcat(dir_patch,"/");
    }
    //G_PRINTF_DEBUG("open dir=%s\n",dir_patch);
    return get_files_from_dir(dir_patch,files[1],MAX_FILES);
}

int sd_mkdir(const TCHAR* path){
    return f_mkdir(path);
}

int sd_open_file(FIL *file,char *file_name,BYTE mode){
    return f_open(file,file_name,mode);
    // Returns a negative error code on failure.
    //int lfs_file_open(lfs_t *lfs, lfs_file_t *file,const char *path, int flags);
   // lfs_file_open(lfs_t *lfs, lfs_file_t *file,const char *path, int flags);
}

int sd_read_file(FIL *file,void* buffer, UINT bytestoread, UINT* bytesreaded){
    return f_read(file,buffer,bytestoread,bytesreaded);
}

int sd_write_file(FIL *file,void* buffer, UINT bytestowrite, UINT* byteswrited){
    return f_write(file,buffer,bytestowrite,byteswrited);
}

int sd_seek_file(FIL *file,FSIZE_t offset){
    return f_lseek(file,offset);
}

int sd_close_file(FIL *file){
    return f_close(file);
}

DWORD sd_file_pos(FIL *file){
    return file->fptr;
}

DWORD sd_file_size(FIL *file){
    return file->obj.objsize;
}

