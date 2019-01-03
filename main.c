#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mysql.h>

#define BACK_LOG 55
#define DEBUG(x) printf("%s%s",x,"\n")
#define ANDROID_OS "android"
#define IPHONE_OS "iphone"
#define IPAD_OS "ipad"
#define BLACKBERRY_OS "blackberry"

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

static MYSQL * ret;


#define SQL_MAX_PATH 255
int init_service_database()
{
       MYSQL * res = mysql_init(NULL);
       if (res) return (SQL_CONNECTED);
       else
         return (SQL_FAILED);
}

int login_socket_api(MYSQL * ptr,int sck,char *usuario,char *pass)
{
       MYSQL * conn = mysql_init(NULL);
       if (!conn) DEBUG("Error initializing database service..");

       if (mysql_real_connect(conn,"localhost","user","password","db",0,NULL,0) == NULL)
       {
             DEBUG("Error connecting with mysql service..");
       }
       int logged = 0;
       char  buffer_sql[SQL_MAX_PATH];
       sprintf(buffer_sql,"SELECT username,password FROM members WHERE username='%s' AND password='%s'",usuario,pass);
       DEBUG("Executing query:");
       DEBUG(buffer_sql);
       if (mysql_query(conn,buffer_sql) )
       {
             DEBUG(mysql_error(conn));
       }
       MYSQL_RES * result = mysql_use_result(conn);
       if (!result)
       {
            char   *buffer = strdup(mysql_error(conn));
            DEBUG(buffer);
       }
       MYSQL_ROW row;
       while ( row = mysql_fetch_row(result) )
       {
           if ( strcmp(usuario,(char*)row[0]) == 0 && strcmp(pass,row[1]) == 0 )
           {
               DEBUG("Login process passed");
               char  buffer[SQL_MAX_PATH];
               sprintf(buffer,"%s","OK");
               send(sck,buffer,strlen(buffer),0);
           }
       }
       


}



void init_server()
{
    server = (sock_server  *  )calloc ( 1, sizeof(sock_server) );

}

void init_socket_client()
{
    cliente = ( sock_client *  )calloc ( 1, sizeof ( sock_client  ) ) ;

}





void createServerSocket(int port)
{
    init_server();
    init_socket_client();

    server->port = port;

    if (server->port)
    {
        server->server_st.sin_addr.s_addr = INADDR_ANY;
        server->server_st.sin_port = htons ( server->port );
        server->server_st.sin_family = AF_INET;
        server->server_socket = socket(AF_INET,SOCK_STREAM,0);
        if (server->server_socket)
        {
            DEBUG("Socket created successfuly..");
        }
    }
}

int binAddr()
{
     int reuse = 1;
     int res = 0;
     if (setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");



    if ( res = bind(server->server_socket,(struct sockaddr*)&server->server_st,sizeof(server->server_st)) == -1 )
    {
         perror("bind");
    }
    if (res) return(res);

}

void listenServer()
{
    listen(server->server_socket,BACK_LOG);
}

void publicarStream(char *in, char  *output,FFmpegConverter *r)
{
    char  arg[1024];
    strcpy(arg," ffmpeg ");
    strcat(arg, "-i ");
    strcat(arg, in);
    strcat(arg, " " );
    strcat(arg,"  -c:v libx264 -rtsp_transport tcp  -crf 23 -preset ultrafast -video_size 320x240 -framerate 25  -bsf:v h264_mp4toannexb -acodec libmp3lame -ar 44100 -ab 1280000  ");
    //exec /usr/local/nginx/ff/ffmpeg -i rtmp://127.0.0.1/in-hls/$name -c:a libfdk_aac -b:a 64k -c:v libx264 -b:v 1000k -f flv -g 30 -r 30 -s 720x54
    strcat(arg, "-f ");
    strcat(arg, "flv "); 
    strcat(arg,output);
    DEBUG(arg);
    //char path[1035];
    //DEBUG(arg);
    char path[1035];
    //DEBUG(arg);
    r->fp = popen(arg,"r");
    if (r->fp)
    {
         while (fgets(path, sizeof(path)-1, r->fp) != NULL) {
                DEBUG(path);
         }
    }else
    {
        DEBUG("Error al abrir comando..");
    }

    

}
void publicarStreamMJPEG(char *in, char  *output,FFmpegConverter *r)
{
    char  arg[1024];
    strcpy(arg," ffmpeg ");
    strcat(arg, "-f mjpeg  -re -i ");
    strcat(arg, in);
    strcat(arg, " " );
    strcat(arg,"  -c:v libx264 -rtsp_transport tcp  -crf 23 -preset ultrafast -video_size 320x240 -framerate 25 -s 320x240 -bsf:v h264_mp4toannexb -acodec libmp3lame -ar 44100 -ab 1280000  ");
    //exec /usr/local/nginx/ff/ffmpeg -i rtmp://127.0.0.1/in-hls/$name -c:a libfdk_aac -b:a 64k -c:v libx264 -b:v 1000k -f flv -g 30 -r 30 -s 720x54
    strcat(arg, "-f ");
    strcat(arg, "rtsp  ");
    strcat(arg,"");
    strcat(arg,output);
    DEBUG(arg);
    //char path[1035];
    //DEBUG(arg);
    char path[1035];
    //DEBUG(arg);
    r->fp = popen(arg,"r");
    if (r->fp)
    {
         while (fgets(path, sizeof(path)-1, r->fp) != NULL) {
                DEBUG(path);
         }
    }else
    {
        DEBUG("Error al abrir comando..");
    }

    

}
void publicarStreamHLS(char *in, char  *output,FFmpegConverter *r)
{
    char  arg[1024];
    strcpy(arg," ffmpeg ");
    strcat(arg, " -i ");
    strcat(arg, in);
    strcat(arg, " " );
    strcat(arg,"  -c:v libx264 -rtsp_transport tcp  -crf 23 -preset ultrafast -video_size 320x240 -framerate 25  -bsf:v h264_mp4toannexb  ");
    //exec /usr/local/nginx/ff/ffmpeg -i rtmp://127.0.0.1/in-hls/$name -c:a libfdk_aac -b:a 64k -c:v libx264 -b:v 1000k -f flv -g 30 -r 30 -s 720x54
    strcat(arg, "-f ");
    strcat(arg, "rtsp  ");
    strcat(arg,"");
    strcat(arg,output);
    DEBUG(arg);
    //char path[1035];
    //DEBUG(arg);
    char path[1035];
    //DEBUG(arg);
    r->fp = popen(arg,"r");
    if (r->fp)
    {
         while (fgets(path, sizeof(path)-1, r->fp) != NULL) {
                DEBUG(path);
         }
    }else
    {
        DEBUG("Error al abrir comando..");
    }

    

}



void grabarCamara(char *in, char  *output,FFmpegConverter *r)
{
    char  arg[1024];
    strcpy(arg,"ffmpeg ");
    strcat(arg, "-i  ");
    strcat(arg, in);
    strcat(arg, " " );
    strcat(arg,"-vcodec ");
    strcat(arg,"copy  ");
    strcat(arg, "-f ");
    strcat(arg, "mpegts ");
    strcat(arg,"-y ");
    strcat(arg,output);
    DEBUG(arg);
    char path[1035];
    DEBUG(arg);
    r->fp = popen(arg,"r");
    if (r->fp)
    {
         while (fgets(path, sizeof(path)-1, r->fp) != NULL) {
                DEBUG(path);
         }
    }else
    {
        DEBUG("Error al abrir comando..");
    }

}

void grabarCamaraMJPEG(char *in, char  *output,FFmpegConverter *r)
{
    char  arg[1024];
    strcpy(arg,"ffmpeg ");
    strcat(arg, "-i  ");
    strcat(arg, in);
    strcat(arg, " " );
    strcat(arg,"-vcodec ");
    strcat(arg,"copy  ");
    strcat(arg, "-f ");
    strcat(arg, "mpegts ");
    strcat(arg,"-y ");
    strcat(arg,output);
    DEBUG(arg);
    char path[1035];
    DEBUG(arg);
    r->fp = popen(arg,"r");
    if (r->fp)
    {
         while (fgets(path, sizeof(path)-1, r->fp) != NULL) {
                DEBUG(path);
         }
    }else
    {
        DEBUG("Error al abrir comando..");
    }

}

void calc_seconds_elapsed()
{
    char buf[1024];
    while (1)
    {
          elapsed_seconds++;
         int sec = elapsed_seconds % 60;
         int minutes = elapsed_seconds / 60;
         int hours = minutes / 60;
         sprintf(buf,"h:%d m:%d s:%d",hours,minutes,sec);
         if (timep)
         {
             timep->buf_time = strdup(buf);
             DEBUG(timep->buf_time);
             sleep(1);
         }

    }
}
int dirExists(const char *path)
{
    struct stat info;

    if(stat( path, &info ) != 0)
        return 0;
    else if(info.st_mode & S_IFDIR)
        return 1;
    else
        return 0;
}




void make_directory(char  *user)
{
      if  ( dirExists(user) == 1 )
      {
             DEBUG("The directory exists..");
      }else
      {
            char    * path = strdup("/var/www/html/");
            char buf[1024];
            strcpy(buf,path);
            strcat(buf,user);
            DEBUG("Creating directory..");
            DEBUG(buf);
            mkdir(buf,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      }
}

void make_directory_guest(char  *user)
{
      if  ( dirExists(user) == 1 )
      {
             DEBUG("The directory exists..");
      }else
      {
            char    * path = strdup("/var/www/html/search/");
            char buf[1024];
            strcpy(buf,path);
            strcat(buf,user);
            DEBUG("Creating directory..");
            DEBUG(buf);
            mkdir(buf,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      }
}
void  myThread(void  *  lpParameter)
{
    FFmpegConverter * r = ( FFmpegConverter *  ) lpParameter;
    char *output = r->output;
    char *entrada = r->input;
    char  *user_name = r->dest;

    if (strstr(output,"rtsp://")  || strstr(output,"rtmp://"))
    {
        DEBUG("Publishing stream..");
        if ( cliente->client_socket)
        {
            //cierro los procesos existentes.
            char  buf[1024];
           // sprintf(buf,"%s","killall -9 ffmpeg");
           // DEBUG("Killing existent processes..");
            //system(buf);
            if ( strstr(r->operating_system,"android") || strstr(r->operating_system,"iphone") || strstr(r->operating_system,"ipad") )
            {
                      DEBUG("Es un movil...");
                      publicarStreamHLS(r->input,r->output,r);
 
            }else
            {
              DEBUG("NO es un movil o una tablet");
              if ( strstr(entrada,".mjpg") || strstr(entrada,".cgi") )
              {
               DEBUG("Retransmitiendo hacia RTSP desde MJPEG..");
               publicarStreamMJPEG(r->input,r->output,r);
              }else
              {
                publicarStreamHLS(r->input,r->output,r);
              }
            }
        }

    }
    else if (strstr(entrada,"http://") || strstr(output,".ts") )
    {
            char    * path = strdup("/var/www/html/");
            char buf[1024];
            strcpy(buf,path);
            strcat(buf,user_name);
            strcat(buf,"/");
            strcat(buf,output);
            DEBUG(buf);
            //calc_seconds_elapsed(cliente->client_socket);
            grabarCamaraMJPEG(r->input,buf,r);
            DEBUG("la entrada es HTTP://");
    }
    else if (strstr(output,".ts") && strcmp(user_name,"invitados") != 0 )
    {
        make_directory(user_name);
        DEBUG("Duplicating source into a output file .ts");
         if ( cliente->client_socket)
        {
            char    * path = strdup("/var/www/html/");
            char buf[1024];
            strcpy(buf,path);
            strcat(buf,user_name);
            strcat(buf,"/");
            strcat(buf,output);
            DEBUG(buf);
            //calc_seconds_elapsed(cliente->client_socket);
            grabarCamara(r->input,buf,r);
        }
  } else if  (strstr(output,".ts") && strcmp(user_name,"invitados") == 0 )
    { 
            make_directory_guest(user_name);
            DEBUG("Es un invitado...grabando como invitado");
            char    * path = strdup("/var/www/html/search/");
            char buf[1024];
            strcpy(buf,path);
            strcat(buf,user_name);
            strcat(buf,"/");
            strcat(buf,output);
            DEBUG(buf);
            //calc_seconds_elapsed(cliente->client_socket);
            grabarCamara(r->input,buf,r);
    }
  

}

void closeProcess()
{
    char  arg[1024];
    strcpy(arg,"killall ffmpeg");
    FILE * f  = popen(arg,"r");
    if ( f )
    {
        DEBUG("Closing ffmpeg process...");
        char path[1024];
         while (fgets(path, sizeof(path)-1, f) != NULL) {
                DEBUG(path);
         }
    }
}

char  *  matchOperatingSystem(char     *buffer)
{ 
      if (strstr(buffer,IPHONE_OS) )
      {
               return (IPHONE_OS);
      }
      if ( strstr(buffer,ANDROID_OS) )
      {
               return (ANDROID_OS);
      }
      if  (strstr(buffer,IPAD_OS) ) 
      { 
               return (IPAD_OS);
      }
      if  (strstr(buffer,BLACKBERRY_OS) )
      {
               return (BLACKBERRY_OS);
      }
}
void acceptCalls()
{
    int server_s;
    char  *token = (char  *  )calloc ( 1, sizeof (char ) ) ;
    while ( 1 )
    {
        int size_socket = sizeof(struct sockaddr_in);
        server_s = server->server_socket;
        if (server_s)
        {
             cliente->client_socket = accept(server_s,(struct sockaddr*)&cliente->cliente_st,&size_socket);
             if (cliente->client_socket)
             {
                 DEBUG("Vidium Server 0.2 stable");
                 char buf[BUFFER_SIZE];
                 int nbytes = 0;
                 char   *match  =  (char  *  )calloc ( 1, sizeof ( char ) ) ; 
                 char  *output =    ( char *  )calloc ( 1, sizeof (char ) ) ;
                 char  *input_file =  (char  *  )calloc ( 1, sizeof ( char )  ) ;
                 char  *user = (char  *  )calloc ( 1, sizeof ( char ) );
                 char  *password = (char  *  )calloc ( 1 ,sizeof ( char ) ) ;
                 int tr = 0;
                 int ot = 0;
                 int login_requested = -1;
                 int ex = 0;
                 if ( nbytes = recv(cliente->client_socket,buf,BUFFER_SIZE,0) )
                 {
                          buf[nbytes] = '\0';
                          if ( buf )
                          {
                                char  *ptr = (char  * )strtok(buf,"|");
                                DEBUG(ptr);
                                while ( ptr != NULL )
                                {
                                       if ( strstr(ptr,"transcode:"))
                                       {
                                          tr = 1;
                                          input_file = strdup(string_replace(ptr,"transcode:",""));
                                       }
                                       if (strstr(ptr,"output_file:"))
                                       {
                                           ot = 1;
                                           output = ( char *  )strdup(string_replace(ptr,"output_file:",""));
                                       }
                                       if (strstr(ptr,"user:") )
                                       {
                                            user = ( char *  )strdup(string_replace(ptr,"user:",""));
                                            user =  (char  * ) strdup(string_replace(user,"'",""));

                                       } 
                                       if (strstr(ptr,"os:") ) 
                                       {
                                            match = strdup(string_replace(ptr,"os",""));
                                            DEBUG("Operating system:");
                                            DEBUG(match);
                                              
         
                                       }
                                       if (strstr(ptr,"login:") )
                                       {
                                          login_requested = 1;
                                          user = (char  *  )strdup(string_replace(ptr,"login:",""));  
                                       }
                                       if (strstr(ptr,"password:") )
                                       {
                                             password = (char  * )strdup ( string_replace(ptr,"password:","") );
           
                                       }
                                       ptr = strtok(NULL,"|");
                                }
                                if (input_file && output && user && login_requested == -1 )
                                {
                                       DEBUG("Processed input:");
                                       DEBUG(input_file);
                                       DEBUG("Processed output:");
                                       DEBUG(output);
                                       DEBUG("User:");
                                       DEBUG(user);
                                }else if (login_requested == 1)
                                {
                                     DEBUG("Login SOCKET api requested..");
                                     DEBUG(user);
                                     DEBUG(password);
                                     //int login_socket_api(MYSQL * ptr,int sck,char *usuario,char *pass)
                                     login_socket_api(ret,cliente->client_socket,user,password);
                                }

                          }


                            if (tr == 1)
                            {
                                 c++;
                                 DEBUG("Thread:");
                                 printf("%d%s",c,"\n");
                                 DEBUG("Transcoding to stream..");
                                 ffmpegTranscoder->input = strdup(input_file);
                                 ffmpegTranscoder->output = strdup(output);
                                 ffmpegTranscoder->dest = strdup(user);
                                 ffmpegTranscoder->operating_system = strdup(match);
                                 pthread_t t[MAX_NUMBER_THREAD];
                                 pthread_create(&t[c],NULL,&myThread,ffmpegTranscoder);
                                 //pthread_join(t,NULL);


                            }

                            if (strcmp(buf,"exit_transcode") == 0 )
                            {
                                DEBUG("Closing process..");
                                closeProcess();
                            }






                 }
                     /*char msg[BUFFER_SIZE];
                     sprintf(msg,"Transcoding URL:%s with output filename:%s",input_file,output);
                     DEBUG(msg);*/


                 }







             }
    }
}



int main()
{

    init_time();
    init_transcoder();
    ret =  init_service_database();
    createServerSocket(30);
    int res = binAddr();
    if (res != -1 )
    {
        DEBUG("bind() call successfuly..");
        listenServer();
        acceptCalls();
    }
    return 0;
}

