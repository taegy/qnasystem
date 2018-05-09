#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <regex>
#include <ctime>
#include <stdlib.h>
#include <curl/curl.h>
//#define CURL_STATICLIB

#define BUFFER_SIZE 255
#define MAX_WORD_SIZE 64

using namespace std;

typedef struct POS {
	char chr;
	char word[MAX_WORD_SIZE];
	char type[MAX_WORD_SIZE];
	char detail[MAX_WORD_SIZE];
} POS;

typedef struct REF {
	char word[MAX_WORD_SIZE];
	char type[MAX_WORD_SIZE];
	char detail[MAX_WORD_SIZE];
} REF;

typedef struct KMA{
	char index[MAX_WORD_SIZE];	// index: 의미없음
	char tm[MAX_WORD_SIZE];		// 발표시간
	char city[MAX_WORD_SIZE];	// 도시
	int hour;					// 시간
	char day[MAX_WORD_SIZE];	// 0: 오늘, 1: 내일, 2: 모레 => yyyy-mm-dd
	char temp[MAX_WORD_SIZE];	// 현재온도
	char tmx[MAX_WORD_SIZE];	// 최고온도
	char tmn[MAX_WORD_SIZE];	// 최저온도
	char sky[MAX_WORD_SIZE];	// 하늘상태[코드]
	char pty[MAX_WORD_SIZE];	// 강수상태 0: 없음, 1: 비, 2: 비/눈, 3: 눈
	char wfKor[MAX_WORD_SIZE];	// 하늘상태[국어]
	char wfEn[MAX_WORD_SIZE];	// 하늘상태[영어]
	char pop[MAX_WORD_SIZE];	// 강수확률 %
	char r12[MAX_WORD_SIZE];	// 12시간 강수량
	char s12[MAX_WORD_SIZE];	// 12시간 강설량
	char ws[MAX_WORD_SIZE];		// 풍속
	char wd[MAX_WORD_SIZE];		// 풍향[코드]
	char wdKor[MAX_WORD_SIZE];	// 풍향[국어]
	char wdEn[MAX_WORD_SIZE];	// 풍향[영어]
	char reh[MAX_WORD_SIZE];	// 습도
	char r06[MAX_WORD_SIZE];	// 6시간 강수량
	char s06[MAX_WORD_SIZE];	// 6시간 강설량
} KMA;

typedef struct KMW{
	char province[MAX_WORD_SIZE];	// 지역구(의미없음)
	char city[MAX_WORD_SIZE];		// 도시
	char numEf[MAX_WORD_SIZE];		// 발표일 기준 날짜 순서
	char tmEf[MAX_WORD_SIZE];		// 날짜
	char wf[MAX_WORD_SIZE];			// 하늘상태[국어]
	char tmx[MAX_WORD_SIZE];		// 최고온도
	char tmn[MAX_WORD_SIZE];		// 최저온도
	char reliability[MAX_WORD_SIZE];// 예보에 대한 신뢰도

} KMW;

REF	*ref_data;
int count_of_ref;
POS	*pos_data;
int	count_of_pos=0;	
KMA *kma_data;
int count_of_kma = 0;
char result_of_qna[MAX_WORD_SIZE];
KMW *kmw_data;
int count_of_kmw = 0;

typedef struct FRAME {
	char name[MAX_WORD_SIZE];
	char focus[MAX_WORD_SIZE];
	char date[2][MAX_WORD_SIZE]; //date[0],2
	char time1[MAX_WORD_SIZE]; //time1
	char time2[MAX_WORD_SIZE]; //time2
	char loc[2][MAX_WORD_SIZE]; //loc[0],2
	char comp[2][MAX_WORD_SIZE]; //comp[0],2
	char res[2][MAX_WORD_SIZE]; //res[0],2
	char person1[MAX_WORD_SIZE]; //person1
	char person2[MAX_WORD_SIZE]; //person2
	char timeract[MAX_WORD_SIZE]; //timeract
	char content[3*MAX_WORD_SIZE]; //content
} FRAME;

FRAME frame;

// 패턴 관련 변수
int sframe = 0;	// 0 : command, 1 : QnA
int spattern = -1; // 날씨는 ppt를 따름, 타이머 10번대, 전화 20번대, 문자 30번대
char dayBefore[MAX_WORD_SIZE];

void updateKMA();
void substr(char *, int, int);
void enterInput(void);
void posTag(void);
void postWork(void);
void regular(void);
int val(POS *obj);
int findPat(void);
void printAll();
int cons(int, int);
void getNextDay(tm *, int);
int regularWeather(POS *);
int qna(char *, char *, char *);
void response(char[], char[], int);

int main(int argc, char **argv[]) {
	// 문장 입력 받기

	updateKMA();
	enterInput();
	posTag();
	postWork();
	regular();	
	findPat();
	printAll();
	system("pause");
	return 0;
}
void substr(char* obj, int offset, int size) {
	char tmp[BUFFER_SIZE];

	// 여기서 오류가 난다면, obj의 크기가 BUFFER_SIZE보다 크기 때문
	strncpy(tmp, obj, strlen(obj)+1);
	if(size > strlen(obj)) {
		strncpy(obj, tmp+offset, strlen(tmp)+1);
	}
	else {
		strncpy(obj, tmp+offset, size);
		obj[size] = '\0';
	}
}
void updateKMA()
{
	char baseurl[128] = "http://www.kma.go.kr/wid/queryDFSRSS.jsp?zone=";
	const int NUMCITY = 17;
	char locationCode[NUMCITY][11] = {"4215061500",      // 강원도
							  "4182025000", // 경기도
							  "4831034000", // 경상남도
							  "4729053000", // 경상북도
							  "2920054000", // 광주
							  "2720065000", // 대구
							  "3023052000", // 대전
							  "2644058000", // 부산
							  "1168066000", // 서울
							  "3611034000", // 세종"
							  "3114056000", // 울산
							  "2871025000", // 인천
							  "4681025000", // 전라남도
							  "4579031000", // 전라북도
							  "5013025300", // 제주
							  "4425051000", // 충청남도
							  "4376031000"  // 충청북도
							  };
	const char location[NUMCITY][20] = {"강원도", "경기도", "경상남도", "경상북도", "광주", "대구",
						  "대전", "부산", "서울", "세종", "울산", "인천", "전라남도",
						  "전라북도", "제주", "충청남도", "충청북도" };

	const char skyKor[4][20] = {"맑음", "구름조금", "구름많음", "흐림"};
	const char windDrection[16][10] = {"E", "동", "N", "북", "NE", "북동", "NW", "북서", "S", "남", "SE", "남동", "SW", "남서", "W", "서"};

	//ofstream fout("kma2.csv", ios::out);
	FILE *kmaData;
	char *webPageBuffer;
	char *ptr;
	char buffer[20];
	CURLcode res;
	int index = 0;   
	int fileSize;
	int is_file_end = 0;
	//remove("kma.txt");

	for(int i=0; i<NUMCITY; i++) {
		
		// get web page
		FILE* webPage = fopen("page.txt", "w+");
		char ct[1024];
		
		char url[128];
		CURL *curl = curl_easy_init();
		strcpy(url, baseurl);
		strcat(url, locationCode[i]);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, webPage);
		res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
		if(res != CURLE_OK) puts("CURL_FAIL!");
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fileSize = ftell(webPage);
		webPageBuffer = (char*)calloc(fileSize+1, sizeof(char));
		fseek(webPage, 0, SEEK_SET);
		char *pagePtr = webPageBuffer;
		for( ; fileSize>0; fileSize-=4096) {
			int res = fread(pagePtr, fileSize>=4096?4096:fileSize, sizeof(char), webPage);
			pagePtr += 4096;
	    }
		pagePtr = pagePtr + fileSize + 1;
		*pagePtr = '\0'; 
		fclose(webPage);
		//puts(webPageBuffer);
		// find starting position
		ptr = webPageBuffer;
		
		kmaData = fopen("kma.txt", "a");
		for(int k=0; k<15; k++) {
			while(ptr != NULL && strncmp(ptr++, "seq", 3) != 0) ;	
					// find and write 19 weather component values
			
		    fprintf(kmaData, "%d|NA|%s", index++, location[i]);
			char skyKorStr[20];
			for(int j=0; j<19; j++) {
				while(*ptr++ != '>') ;
				while(*ptr++ != '>') ;
				char *end = ptr;
				while(*end != '<') ++end;
				*end = '\0'; // componentValue = >*\0w
				
				if(j==1) {
					time_t now = time(NULL);
					tm *gmtm = localtime(&now);
					char dateStr[MAX_WORD_SIZE];
					char dt[BUFFER_SIZE];
					getNextDay(gmtm, atoi(ptr)); // 0 1 2
					itoa(1900+gmtm->tm_year, dateStr, 10);
					strcat(dateStr, "-");
					itoa(gmtm->tm_mon+1, dt, 10);
					strcat(dateStr, dt);
					strcat(dateStr, "-");
					itoa(gmtm->tm_mday, dt, 10);
					strcat(dateStr, dt);
					fprintf(kmaData, "|%s", dateStr);
				}else if(j==5) {
					 strcpy(skyKorStr, skyKor[atoi(ptr)]);
					 fprintf(kmaData, "|%s", ptr);
				}
				else if(j==7)
					 fprintf(kmaData, "|%s", skyKorStr);
				else if(j==8)
					fprintf(kmaData, "|skyEn");
				else if(j==14) {}
				else if(j==15) {
					char wdKor[10];
					for(int n=0; n<8; n++)
						if(strcmp(ptr, windDrection[2*n]) == 0)
							strcpy(wdKor, windDrection[2*n+1]);
							
					fprintf(kmaData, "|%s|%s", wdKor, ptr);
				}
				else
					fprintf(kmaData, "|%s", ptr);
				fflush(kmaData);
			}
			fprintf(kmaData, "\n");
			fflush(kmaData);
		}
		fclose(kmaData);
		remove("page.txt");
	}
}

void enterInput(void) {
	// 문장 입력 받기
	char tmp[BUFFER_SIZE];
	FILE* fp = fopen("in.txt", "w");
	printf("Input: ");
	scanf("%[^\n]s", tmp);
	fprintf(fp, tmp);
	fclose(fp);
	// 형태소 분석기 실행
	system("kma -1 in.txt > process.txt");
}
void posTag(void) {
	FILE *fp;
	int i, count_of_str;
	char tmp[BUFFER_SIZE], buf1[MAX_WORD_SIZE], buf2[MAX_WORD_SIZE], *ptr;

	fp = fopen("process.txt", "r");

	count_of_str = 0;
	// 단어 개수 세기
	while( !feof(fp) ) { 
		fscanf(fp, "%s", tmp);
		++count_of_str;
	}

	if(pos_data == NULL) {
		pos_data = (POS*)malloc(sizeof(POS)*count_of_str);
	}

	if(fp) {
		fseek(fp, 0, SEEK_SET);
		fscanf( fp, "%s", buf1);
		while( fscanf( fp, "%s", buf2) > 0) {
			if( buf1[0]=='(' && ( (buf2[strlen(buf2)-2]==')' && buf2[strlen(buf2)-1]=='<') || buf2[strlen(buf2)-1]=='>')) {

				pos_data[count_of_pos].chr = buf1[1];

				ptr = strstr( buf2,"\")<");
				memset( ptr,0,strlen(ptr));
				strcpy( pos_data[count_of_pos].word, buf2+1);
				strcpy( pos_data[count_of_pos].type, "");
				strcpy( pos_data[count_of_pos].detail, "");
				count_of_pos++;
			}
			else if( buf1[0]=='(' && buf2[strlen(buf2)-1]==')') {

				pos_data[count_of_pos].chr = buf1[1];

				ptr = strstr( buf2,"\")");
				memset( ptr,0,strlen(ptr));
				strcpy( pos_data[count_of_pos].word, buf2+1);
				strcpy( pos_data[count_of_pos].type, "");
				strcpy( pos_data[count_of_pos].detail, "");

				count_of_pos++;
			}

			strcpy( buf1,buf2);
			memset( buf2,0,MAX_WORD_SIZE);
		}
		fclose(fp);

		//for(i = 0; i < count_of_pos; i++) printf("%s, %c\n", pos_data[i].word, pos_data[i].chr); // trace
	}
}
void postWork(void) {
	int i;
	regex rxd("[1][0-2]월|[0-9]\\월|[0-9]+개월|[0-9]+일|[1]*[0-9]시|[2][0-4]시");
	regex rxt("[0-9]+시간|[0-9]+분|[0-9]+초");
	regex rxp("[0][0-9]{1,2}\\-[0-9]{3,4}\\-[0-9]{4}");

	// 개체명 인식기 적용
	for(i = 0; i < count_of_pos; i++) {
		val(pos_data+i);
		if(strcmp(pos_data[i].detail,"타이머제어")==0) strcpy(frame.timeract, pos_data[i].word);
		if(strcmp(pos_data[i].detail,"LOCATION")==0) 
			strcmp(frame.loc[0],"")==0 ? strcpy(frame.loc[0], pos_data[i].word) : strcpy(frame.loc[1], pos_data[i].word);
		if(strcmp(pos_data[i].detail,"PERSON")==0) 
			strcmp(frame.person1,"")==0 ? strcpy(frame.person1, pos_data[i].word) : strcpy(frame.person2, pos_data[i].word);
	}

	for( i=0; i< count_of_pos; i++) {
		if( pos_data[i].chr == 'N' ) {
			// phone 
			if( regex_match(pos_data[i].word, rxp )) {
				strcpy(pos_data[i].detail, "PERSON");
			}
			// time 
			else if(regex_match(pos_data[i].word, rxd)) {
				strcpy(pos_data[i].detail, "DATE");	

			}
			else if(regex_match(pos_data[i].word, rxt)) {
				strcpy(pos_data[i].detail, "TIME");
				strcmp(frame.time1,"")==0 ? strcpy(frame.time1, pos_data[i].word) : strcpy(frame.time2, pos_data[i].word);
			}
			else if((strcmp(pos_data[i].word , "강수") == 0) || (strcmp(pos_data[i].word , "강설") == 0)) {
				if(strcmp(pos_data[i+1].word , "확률")==0 || strcmp(pos_data[i+1].word , "상태")== 0) {
					cons(i, i+1);
					val(pos_data+i);
				}
			}
			else if((strcmp(pos_data[i].word , "최저온") == 0) || (strcmp(pos_data[i].word , "최고온") == 0)) {
				if(strcmp(pos_data[i+1].word , "도")==0 && pos_data[i+1].chr == 'j') {
					cons(i, i+1);
					val(pos_data+i);
				}
			}
		}
		else if( pos_data[i].chr == 'V') {

			if((strcmp(pos_data[i].word , "메") == 0) && (strcmp(pos_data[i+1].word , "시") == 0) && (strncmp(pos_data[i+2].word, "지", 2) == 0)) {
				cons(i, i+1);
				cons(i, i+1);
				val(pos_data+i);
			}
			else {
				strcpy(pos_data[i].type, "ACTION");
				val(pos_data+i);
			}
		}
		else if( pos_data[i].chr == 'j') {
			if((strcmp(pos_data[i].word,"으로부터")==0) || (strcmp(pos_data[i].word,"로부터")==0) || (strcmp(pos_data[i].word,"한테서")==0)) {
				strcpy(pos_data[i].detail, "FROM");
			}
			else if((strcmp(pos_data[i].word,"으로")==0) || (strcmp(pos_data[i].word,"한테")==0) || (strcmp(pos_data[i].word,"에게")==0) || (strcmp(pos_data[i].word,"께서")==0)) {
				strcpy(pos_data[i].detail, "TO");
			}
			else if((strcmp(pos_data[i].word,"를")==0) || (strcmp(pos_data[i].word,"을")==0)) {
				strcpy(pos_data[i].detail, "OJB");
			}
			else if((strcmp(pos_data[i].word,"의")==0)) {
				strcpy(pos_data[i].detail, "OF");
			}
			else {
				strcpy(pos_data[i].detail , "");
			}

		}
		else if( pos_data[i].chr == 'Z') {
			if((strcmp(pos_data[i].word,"몇")==0) && strncmp(pos_data[i+1].word,"도",2) == 0) {
				strcat(pos_data[i].word,"도"); 
				if(strlen(pos_data[i].word) == 2) {
					strcpy(pos_data[i+1].word, "");
					cons(i,i+1);
				} else {
					substr(pos_data[i+1].word, 2, strlen(pos_data[i+1].word));
				}
				val(pos_data+i);

			} else if((strcmp(pos_data[i].word,"몇")==0) && strcmp(pos_data[i+1].word,"돌") == 0 && strncmp(pos_data[i+2].word,"니",2) == 0) {
				strcat(pos_data[i].word,"도"); 
				strcpy(pos_data[i+1].word, "");
				cons(i,i+1);
				val(pos_data+i);
			}
		}
		else {
			strcpy(pos_data[i].detail , "");
			val(pos_data+i);
		}
	}
}
void regular(void) {
	time_t now = time(NULL);
	int i, j; char dt[BUFFER_SIZE];
	char tmp[BUFFER_SIZE];
	tm *gmtm = localtime(&now);

	for(i = 0; i < count_of_pos; i++) {
		// DATE 정규화
		
		if((strcmp(pos_data[i].word,"오늘") == 0) || (strcmp(pos_data[i].word,"현재") == 0))  {

			getNextDay(gmtm, -1);
			itoa(1900+gmtm->tm_year, dayBefore, 10);
			strcat(dayBefore, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(dayBefore, dt);
			strcat(dayBefore, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(dayBefore, dt);
			strcat(dayBefore, "일");

			getNextDay(gmtm, 0);
			itoa(1900+gmtm->tm_year, pos_data[i].word, 10);
			strcat(pos_data[i].word, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "일");

			if((strcmp(pos_data[i+1].word,"오후") == 0) || (strcmp(pos_data[i+1].word,"저녁") == 0) || (strcmp(pos_data[i+1].word,"낮") == 0)){
				// 4 시 같은 시가 있다면 12를 더한다
				if(strcmp(pos_data[i+2].detail, "DATE") == 0) {
					strcpy(pos_data[i+1].word, "");
					cons(i+1, i+2);
					substr(pos_data[i+1].word, 0, strlen(pos_data[i+1].word)-2);
					itoa(atoi(pos_data[i+1].word)+12, pos_data[i+1].word, 10);
					strcat(pos_data[i+1].word, "시");
				} else {
					strcpy(pos_data[i+1].word, "14시");
				}
				strcat(pos_data[i].word, " ");
				cons(i, i+1);
			} else if(strcmp(pos_data[i+1].word,"오전") == 0){
				if(strcmp(pos_data[i+2].detail, "DATE") == 0) {
					strcpy(pos_data[i+1].word, "");
					cons(i+1, i+2);
				} else {
					strcpy(pos_data[i+1].word, "8시");
				}
				strcat(pos_data[i].word, " ");
				cons(i, i+1);
			} else if(strcmp(pos_data[i+1].detail, "DATE")==0){ // 오늘 2시 등을 처리하기 위한
				strcpy(tmp, pos_data[i+1].word);
				if(tmp[0] >= '0' && tmp[0] <= '9') {
					tmp[0] = tmp[strlen(tmp)-2];
					tmp[1] = tmp[strlen(tmp)-1];
					tmp[2] = '\0';
					if(strcmp("시", tmp) == 0) {
						strcat(pos_data[i].word, " ");
						cons(i,i+1);
					}
				}
			}			
		} else if(strcmp(pos_data[i].word,"내일") == 0){

			getNextDay(gmtm, 0);
			itoa(1900+gmtm->tm_year, dayBefore, 10);
			strcat(dayBefore, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(dayBefore, dt);
			strcat(dayBefore, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(dayBefore, dt);
			strcat(dayBefore, "일");

			getNextDay(gmtm, 1);
			itoa(1900+gmtm->tm_year, pos_data[i].word, 10);
			strcat(pos_data[i].word, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "일");
			if((strcmp(pos_data[i+1].word,"오후") == 0) || (strcmp(pos_data[i+1].word,"저녁") == 0) || (strcmp(pos_data[i+1].word,"낮") == 0)){
				// 4 시 같은 시가 있다면 12를 더한다
				if(strcmp(pos_data[i+2].detail, "DATE") == 0) {
					strcpy(pos_data[i+1].word, "");
					cons(i+1, i+2);
					substr(pos_data[i+1].word, 0, strlen(pos_data[i+1].word)-2);
					itoa(atoi(pos_data[i+1].word)+12, pos_data[i+1].word, 10);
					strcat(pos_data[i+1].word, "시");
				} else {
					strcpy(pos_data[i+1].word, "14시");
				}
				strcat(pos_data[i].word, " ");
				cons(i, i+1);
			} else if(strcmp(pos_data[i+1].word,"오전") == 0){
				if(strcmp(pos_data[i+2].detail, "DATE") == 0) {
					strcpy(pos_data[i+1].word, "");
					cons(i+1, i+2);
				} else {
					strcpy(pos_data[i+1].word, "8시");
				}
				strcat(pos_data[i].word, " ");
				cons(i, i+1);
			} else if(strcmp(pos_data[i+1].detail, "DATE")==0){ // 오늘 2시 등을 처리하기 위한
				strcpy(tmp, pos_data[i+1].word);
				if(tmp[0] >= '0' && tmp[0] <= '9') {
					tmp[0] = tmp[strlen(tmp)-2];
					tmp[1] = tmp[strlen(tmp)-1];
					tmp[2] = '\0';
					if(strcmp("시", tmp) == 0) {
						strcat(pos_data[i].word, " ");
						cons(i,i+1);
					}
				}
			}
		} else if(strcmp(pos_data[i].word,"모레") == 0){

			getNextDay(gmtm, 1);
			itoa(1900+gmtm->tm_year, dayBefore, 10);
			strcat(dayBefore, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(dayBefore, dt);
			strcat(dayBefore, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(dayBefore, dt);
			strcat(dayBefore, "일");
			
			getNextDay(gmtm, 2);
			itoa(1900+gmtm->tm_year, pos_data[i].word, 10);
			strcat(pos_data[i].word, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "일");
		} else if(strcmp(pos_data[i].word,"글피") == 0){
			getNextDay(gmtm, 3);
			itoa(1900+gmtm->tm_year, pos_data[i].word, 10);
			strcat(pos_data[i].word, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "일");
		} else if(strcmp(pos_data[i].word,"어제") == 0 || strcmp(pos_data[i].word,"어저께") == 0){
			getNextDay(gmtm, -1);
			itoa(1900+gmtm->tm_year, pos_data[i].word, 10);
			strcat(pos_data[i].word, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "일 ");
		} else if(strcmp(pos_data[i].word,"그제") == 0 || strcmp(pos_data[i].word,"그저께") == 0){
			getNextDay(gmtm, -2);
			itoa(1900+gmtm->tm_year, pos_data[i].word, 10);
			strcat(pos_data[i].word, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "일");
		} else if( strcmp(pos_data[i].word,"오후") == 0) {
			getNextDay(gmtm, 0);
			itoa(1900+gmtm->tm_year, pos_data[i].word, 10);
			strcat(pos_data[i].word, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "일");
			// 4 시 같은 시가 있다면 12를 더한다
			if(strcmp(pos_data[i+1].detail, "DATE") == 0) {
				substr(pos_data[i+1].word, 0, strlen(pos_data[i+1].word)-2);
				itoa(atoi(pos_data[i+1].word)+12, pos_data[i+1].word, 10);
				strcat(pos_data[i+1].word, "시");
				strcat(pos_data[i].word, " ");
				cons(i, i+1);
			} else {
				strcat(pos_data[i].word, " 14시");
			}
		} else if( strcmp(pos_data[i].word,"오전") == 0) {
			getNextDay(gmtm, 0);
			itoa(1900+gmtm->tm_year, pos_data[i].word, 10);
			strcat(pos_data[i].word, "년 ");
			itoa(gmtm->tm_mon+1, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "월 ");
			itoa(gmtm->tm_mday, dt, 10);
			strcat(pos_data[i].word, dt);
			strcat(pos_data[i].word, "일");
			if(strcmp(pos_data[i+1].detail, "DATE") == 0) {
				strcat(pos_data[i].word, " ");
				cons(i, i+1);
			} else {
				strcat(pos_data[i].word, " 8시");
			}
		}  else if(strcmp(pos_data[i].detail, "DATE")==0){ // 오늘 2시 등을 처리하기 위한
			strcpy(tmp, pos_data[i].word);
			if(tmp[0] >= '0' && tmp[0] <= '9') {
				tmp[0] = tmp[strlen(tmp)-2];
				tmp[1] = tmp[strlen(tmp)-1];
				tmp[2] = '\0';
				if(strcmp("시", tmp) == 0) {
					strcpy(tmp, pos_data[i].word);
					getNextDay(gmtm, 0);
					itoa(1900+gmtm->tm_year, pos_data[i].word, 10);
					strcat(pos_data[i].word, "년 ");
					itoa(gmtm->tm_mon+1, dt, 10);
					strcat(pos_data[i].word, dt);
					strcat(pos_data[i].word, "월 ");
					itoa(gmtm->tm_mday, dt, 10);
					strcat(pos_data[i].word, dt);
					strcat(pos_data[i].word, "일 ");
					strcat(pos_data[i].word, tmp);
				}
			}
		}
		if(strcmp(pos_data[i].detail,"DATE")==0)
			strcmp(frame.date[0],"")==0 ? strcpy(frame.date[0], pos_data[i].word) : strcpy(frame.date[1], pos_data[i].word);
	}
	// 날씨, TIME 정규화
	for(i = 0; i < count_of_pos; i++) {
		if((strcmp(pos_data[i].detail, "TIME")==0) && (strcmp(pos_data[i+1].detail, "TIME") == 0)) {
			strcat(pos_data[i].word, " ");
			cons(i, i+1);
			i--;
		} else if((strcmp(pos_data[i].type,"날씨") == 0)) {
			strcpy(pos_data[i].word, pos_data[i].detail);
			if(!(strcmp(pos_data[i].detail,"비") == 0 || strcmp(pos_data[i].detail,"눈") == 0 ||strcmp(pos_data[i].detail,"바람") == 0))
				strcmp(frame.comp[0],"")==0 ? strcpy(frame.comp[0], pos_data[i].word) : strcpy(frame.comp[1], pos_data[i].word);
		}
	}
}
int val(POS *obj) {
	char tmp[BUFFER_SIZE];
	int div, i, j, num_of_work;
	FILE *fp;

	// 이 if문은 데이터베이스를 읽어오는 부분
	if(ref_data == NULL) {
		fp = fopen("db.txt", "r");
		count_of_ref = 0;
		while(!feof(fp)) {
			fscanf(fp, "%s", tmp);
			count_of_ref++;	
		}
		ref_data = (REF*)malloc(sizeof(REF)*count_of_ref);
		fseek(fp, 0, SEEK_SET);
		// db.txt 한 줄씩 돌아가면서
		for(i = 0; i < count_of_ref; i++) {
			fscanf(fp, "%s", tmp);
			div = 0;
			num_of_work = 0;
			for(j = 0; j < strlen(tmp); j++) {
				if(tmp[j] == '|') {
					switch(num_of_work) {
					case 0:
						strncpy(ref_data[i].word, tmp+div, j-div);							
						ref_data[i].word[j-div] = '\0';
						break;
					case 1:
						strncpy(ref_data[i].type, tmp+div, j-div);							
						ref_data[i].type[j-div] = '\0';
						break;
					case 2:
						strncpy(ref_data[i].detail, tmp+div, j-div);
						ref_data[i].detail[j-div] = '\0';
						break;
					}
					num_of_work++;
					div = j + 1;					
				}
			}
			//printf("%s, %s, %s\n",ref_data[i].word, ref_data[i].type, ref_data[i].detail); // for trace
		}
		fclose(fp);
	}

	for(i = 0; i < count_of_ref; i++) {
		if(strcmp(ref_data[i].word, obj->word) == 0) {
			strcpy(obj->type, ref_data[i].type);
			strcpy(obj->detail, ref_data[i].detail);
			//printf("%s, %c, %s, %s\n", obj->word, obj->chr, obj->type, obj->detail); // for trace
			return 0;
		}		
	}

	// 검색 값이 없을 경우
	return -1;
}
int findPat(void) {
	int i, j, count_of_people = 0, count_of_location = 0, count_of_date = 0;


	if(strcmp(pos_data[0].detail, "LOCATION") == 0 || strcmp(pos_data[0].detail, "DATE") == 0) {
		sframe = 1; // 문장의 프레임을 QnA로 설정한다.
		spattern = 0; // 날씨에 관한 것이라는 것을 알려준다.
	}

	if(sframe == 0) {
		// 전화 패턴 확인
		for(i = 0; i < count_of_pos; i++) {
			if(strcmp(pos_data[i].type, "전화") == 0) {
				// 전화 걸기 패턴 확인
				for(j = 0; j < count_of_pos; j++) {
					if(strcmp(pos_data[j].detail, "PERSON") == 0) {
						spattern = 10; // 전화와 PERSON이 있으면 10번 패턴을 찾았다고 설정
						return 1;
					}
				}
				// 전화 끊기 패턴 확인
				for(j = 0; j < count_of_pos; j++) {
					if(strcmp(pos_data[j].detail, "종료") == 0) {
						spattern = 11; // 전화와 PERSON이 있으면 11번 패턴을 찾았다고 설정
						return 1;
					}
				}
				spattern = -1; // 전화만 찾고 PERSON을 찾지 못하면 에러코드 출력
			}
		}
		// 타이머 패턴 확인
		for(i = 0; i < count_of_pos; i++) {			
			if(strcmp(pos_data[i].type, "타이머") == 0) {
				// 타이머 설정 패턴 확인
				for(j = 0; j < count_of_pos; j++) {
					if(strcmp(pos_data[j].detail, "TIME") == 0) {
						spattern = 20; // 타이머와 TIME이 있으면 20번 패턴을 찾았다고 설정
						return 1;
					}					
				}
				// 타이머 조작 패턴 확인
				for(j = 0; j < count_of_pos; j++) {
					if(strcmp(pos_data[j].detail, "타이머제어") == 0) {
						spattern = 21; // 타이머와 ACTION이 있으면 21번 패턴을 찾았다고 설정
						return 1;
					}					
				}
				spattern = -1; // 전화만 찾고 PERSON을 찾지 못하면 에러코드 출력
			}
		}
		// 문자 패턴 확인
		// ~에게, ~라고 문자 보내. 또는 ~에게 문자 보내, 라고 또는 
		// content(~라고 앞 내용)가 안 잡히면 문자 이후에 content(문자 ~/문자 보내 ~)가 있는것으로 생각
		if(strcmp(pos_data[0].detail,"PERSON")==0) {
			count_of_people=1;
			for(i = 1; i < count_of_pos; i++) {
				if(strcmp(pos_data[i].detail,"PERSON")==0) {
					count_of_people++;
				} else if((strcmp(pos_data[i].type,"문자")==0) && ((strcmp(pos_data[i+1].detail,"전송")==0) || (strcmp(pos_data[i+2].detail,"전송")==0))) {
					if(count_of_people > 1) {
						spattern = 32;
						return 1;
					} else if (count_of_people == 1) {
						spattern = 31;
					}
				} else if((strcmp(pos_data[i].type,"문자")==0) && ((strcmp(pos_data[i+1].detail,"읽기")==0) || (strcmp(pos_data[i+2].detail,"읽기")==0))) {
					if(count_of_people == 1) {
						spattern = 33;
					} else if(count_of_people > 1) {
						spattern = -1;
					}
					return 1;
				} else if(strcmp(pos_data[i].type,"문자")==0) {
					if(count_of_people == 1) {
						spattern = 31;
					} else if(count_of_people > 1) {
						spattern = 32;
					}
				}
				// 황태경에게 ~라고 문자 보내, 황태경에게 문자 보내 ~라고
				// 황태경에게 문자 보내라고 문자 보내, 황태경에게 문자 보내 문자 보내라고
				if(strcmp(pos_data[i].word,"라고")==0) {
					char content[3*MAX_WORD_SIZE]="\0";
					for(int j=2; j<i; j++) {
						strcat(content, pos_data[j].word);
						if(j<i-1)
							strcat(content, " ");
					}

					if(i == count_of_pos-1) // 마지막 word가 라고
						strcpy(frame.content,content+strlen("문자 보내 "));
					else 
						strcpy(frame.content,content);
				}
			}
		}
	}
	else if(sframe == 1) {
		for(i = 0; i < count_of_pos; i++) {
			if(strcmp(pos_data[i].detail,"LOCATION")==0) {
				count_of_location++;
			} else if(strcmp(pos_data[i].detail,"DATE")==0) {
				count_of_date++;
				//tkhwang : 눈, 비, 바람 -> 날씨요소질의2 spattern=1
			} else if(strcmp( pos_data[i].detail,"눈")==0 || strcmp( pos_data[i].detail,"비")==0 || strcmp( pos_data[i].detail,"바람")==0) {
				if(strcmp( pos_data[i].detail,"눈")==0) {
					strcpy(frame.focus,"날씨요소(눈)");
					strcpy(frame.comp[0],"강설량");
					strcpy(frame.comp[1],"강설확률");
				}else if(strcmp( pos_data[i].detail,"비")==0) {
					strcpy(frame.focus,"날씨요소(비)");
					strcpy(frame.comp[0],"강수량");
					strcpy(frame.comp[1],"강수확률");
				}else if(strcmp( pos_data[i].detail,"바람")==0) {
					strcpy(frame.focus,"날씨요소(바람)");
					strcpy(frame.comp[0],"풍속");
					strcpy(frame.comp[1],"풍향");
				}
				spattern = 1;
				//return 1; 비교질의일수도 있으므로 리턴 안함
			}else if(strcmp(pos_data[i].detail,"날씨")==0) {
				spattern += 4;
				// 내일 날씨 더워? 처리 하도록, 
			}
			if(spattern >=4 && strcmp(pos_data[i].type,"날씨") == 0 &&
				strcmp(pos_data[i].detail,"날씨") != 0) {
				spattern -= 4; // spat0 또는 spat1로 돌아감
				// 내일 '날씨' 부산이랑 서울 중에 어디가 '더워'? 의 경우를 대비해 리턴하지 않음.
			}
		}
		if(count_of_location > 1) {
			spattern = 2;
			return 1;
		} else if(count_of_date > 1) {
			spattern = 3;
			return 1;
		} else {
			if(spattern==5) spattern=4;
			return 1;
		}
	}
	return -1;
}

void response(char *name, char *focus, int qnaNum) {
	FILE *ff;
	char upperComp[MAX_WORD_SIZE] = "";

	ff = fopen("frameOutput.txt", "w");

	if(strcmp(frame.comp[0], "강수량") == 0)
		strcpy(upperComp, "비");
	else if(strcmp(frame.comp[0], "강설량") == 0)
		strcpy(upperComp, "눈");
	else if(strcmp(frame.comp[0], "풍속") == 0)
		strcpy(upperComp, "바람");

	strcpy(frame.name,name);
	strcpy(frame.focus,focus);
	strcat(frame.focus, strcmp(upperComp,"")==0 ? frame.comp[0] : upperComp);

	for(int i=0; i<qnaNum; i++) {
		//단순날씨와 그외
		//qna(frame.comp[i], frame.date[i], frame.loc[i]);
		if(strcmp(frame.name,"단순날씨구성요소")==0) {
			qna(frame.comp[i], frame.date[i], frame.loc[i]);
		} else if(strcmp(frame.name,"복합날씨구성요소")==0) {
			qna(frame.comp[i], frame.date[0], frame.loc[0]);
		} else if(strcmp(frame.name,"단순날씨구성요소지역비교")==0) {
			qna(frame.comp[0], frame.date[0], frame.loc[i]);
		} else if(strcmp(frame.name,"단순날씨구성요소날짜비교")==0) {
			qna(frame.comp[0], frame.date[i], frame.loc[0]);
		} 
		strcpy(frame.res[i], result_of_qna);
	}
	strcat(frame.focus,")");
	printf("NAME:%s\n",frame.name);
	fprintf(ff, "NAME:%s\n",frame.name);
	printf("FOCUS:%s\n",frame.focus);
	fprintf(ff,"FOCUS:%s\n",frame.focus);

	if(strcmp(frame.name,"단순날씨구성요소")==0) {
		printf("$date:%s\n$loc:%s\n$%s:%s\n",frame.date[0],frame.loc[0],frame.comp[0],frame.res[0]);
		fprintf(ff, "$date:%s\n$loc:%s\n$%s:%s\n",frame.date[0],frame.loc[0],frame.comp[0],frame.res[0]);
		printf("%s %s의 %s은(는) '%s'입니다\n",frame.date[0],frame.loc[0],frame.comp[0],frame.res[0]);
		fprintf(ff,"%s %s의 %s은(는) '%s'입니다\n",frame.date[0],frame.loc[0],frame.comp[0],frame.res[0]);
	}else if(strcmp(frame.name,"복합날씨구성요소")==0) {
		printf("$date:%s\n$loc:%s\n$%s:%s\n$%s:%s\n",frame.date[0],frame.loc[0],frame.comp[0],frame.res[0],frame.comp[1],frame.res[1]);
		fprintf(ff,"$date:%s\n$loc:%s\n$%s:%s\n$%s:%s\n",frame.date[0],frame.loc[0],frame.comp[0],frame.res[0],frame.comp[1],frame.res[1]);
		printf("%s %s %s은(는) '%s'이고 %s은(는) '%s'입니다.\n",frame.date[0],frame.loc[0],frame.comp[0],frame.res[0],frame.comp[1],frame.res[1]);
		fprintf(ff,"%s %s %s은(는) '%s'이고 %s은(는) '%s'입니다.\n",frame.date[0],frame.loc[0],frame.comp[0],frame.res[0],frame.comp[1],frame.res[1]);
	}else if(strcmp(frame.name,"단순날씨구성요소지역비교")==0) {
		printf("$date:%s\n$loc[0]:%s\n$loc[1]:%s\n$%s1:%s\n$%s2:%s\n",
			frame.date[0],frame.loc[0],frame.loc[1],frame.comp[0],frame.res[0],frame.comp[0],frame.res[1]);
		fprintf(ff,"$date:%s\n$loc[0]:%s\n$loc[1]:%s\n$%s1:%s\n$%s2:%s\n",
			frame.date[0],frame.loc[0],frame.loc[1],frame.comp[0],frame.res[0],frame.comp[0],frame.res[1]);
		if(strcmp(frame.res[0], frame.res[1])!=0) {
			printf("%s %s보다 %s이(가) '%s'이(가) 더 높습니다\n",frame.date[0],(frame.res[0][0]=='-'&&frame.res[1][0]=='-') ? (strcmp(frame.res[0],frame.res[1]) > 0 ? frame.loc[0] : frame.loc[1]) : (strcmp(frame.res[0],frame.res[1]) < 0 ? frame.loc[0] : frame.loc[1]),
			(frame.res[0][0]=='-'&&frame.res[1][0]=='-') ? (strcmp(frame.res[0],frame.res[1]) < 0 ? frame.loc[0] : frame.loc[1]) : (strcmp(frame.res[0],frame.res[1]) > 0 ? frame.loc[0] : frame.loc[1]), frame.comp[0]);
		fprintf(ff,"%s %s보다 %s이(가) '%s'이(가) 더 높습니다\n",frame.date[0],(frame.res[0][0]=='-'&&frame.res[1][0]=='-') ? (strcmp(frame.res[0],frame.res[1]) > 0 ? frame.loc[0] : frame.loc[1]) : (strcmp(frame.res[0],frame.res[1]) < 0 ? frame.loc[0] : frame.loc[1]),
			(frame.res[0][0]=='-'&&frame.res[1][0]=='-') ? (strcmp(frame.res[0],frame.res[1]) < 0 ? frame.loc[0] : frame.loc[1]) : (strcmp(frame.res[0],frame.res[1]) > 0 ? frame.loc[0] : frame.loc[1]), frame.comp[0]);
		}
		else {
			printf("%s %s과 %s의 %s는 같습니다\n",frame.date[0],frame.loc[0],frame.loc[1],frame.comp[0]);
			fprintf(ff,"%s %s과 %s의 %s는 같습니다\n",frame.date[0],frame.loc[0],frame.loc[1],frame.comp[0]);
		}
	}else if(strcmp(frame.name,"단순날씨구성요소날짜비교")==0) {
		printf("$loc:%s\n$date[0]:%s\n$date[1]:%s\n$%s1:%s\n$%s2:%s\n",
			frame.loc[0],frame.date[0],frame.date[1],frame.comp[0],frame.res[0],frame.comp[0],frame.res[1]);
		
		fprintf(ff,"$loc:%s\n$date[0]:%s\n$date[1]:%s\n$%s1:%s\n$%s2:%s\n",
			frame.loc[0],frame.date[0],frame.date[1],frame.comp[0],frame.res[0],frame.comp[0],frame.res[1]);
		if(strcmp(frame.res[0], frame.res[1])!=0) {
			printf("%s %s보다 %s이(가) %s가 더 높습니다\n",frame.loc[0], (frame.res[0][0]=='-'&&frame.res[1][0]=='-') ? (strcmp(frame.res[0],frame.res[1]) > 0 ? frame.date[0] : frame.date[1]) : (strcmp(frame.res[0],frame.res[1]) < 0 ? frame.date[0] : frame.date[1]),
			(frame.res[0][0]=='-'&&frame.res[1][0]=='-') ? (strcmp(frame.res[0],frame.res[1]) < 0 ? frame.date[0] : frame.date[1]) : (strcmp(frame.res[0],frame.res[1]) > 0 ? frame.date[0] : frame.date[1]), frame.comp[0]);
			fprintf(ff,"%s %s보다 %s이(가) %s가 더 높습니다\n",frame.loc[0], (frame.res[0][0]=='-'&&frame.res[1][0]=='-') ? (strcmp(frame.res[0],frame.res[1]) > 0 ? frame.date[0] : frame.date[1]) : (strcmp(frame.res[0],frame.res[1]) < 0 ? frame.date[0] : frame.date[1]),
			(frame.res[0][0]=='-'&&frame.res[1][0]=='-') ? (strcmp(frame.res[0],frame.res[1]) < 0 ? frame.date[0] : frame.date[1]) : (strcmp(frame.res[0],frame.res[1]) > 0 ? frame.date[0] : frame.date[1]), frame.comp[0]);
		}
		else{
			printf("%s %s과 %s의 %s는 같습니다\n",frame.loc[0],frame.date[0],frame.date[1],frame.comp[0]);
			fprintf(ff,"%s %s과 %s의 %s는 같습니다\n",frame.loc[0],frame.date[0],frame.date[1],frame.comp[0]);
		}

	}else if(strcmp(frame.name,"종합날씨")==0) {

		char skyStat1[MAX_WORD_SIZE], skyStat2[MAX_WORD_SIZE],
			am[MAX_WORD_SIZE], pm[MAX_WORD_SIZE], rainAmt[MAX_WORD_SIZE], hTemp1[MAX_WORD_SIZE], hTemp2[MAX_WORD_SIZE],
			pm2[MAX_WORD_SIZE];

		strcpy(am, frame.date[0]);
		strcat(am, " 8시");
		qna("하늘상태", am, frame.loc[0]);
		strcpy(skyStat1, result_of_qna);

		strcpy(pm, frame.date[0]);
		strcat(pm, " 14시");
		qna("하늘상태", pm, frame.loc[1]);
		strcpy(skyStat2, result_of_qna);

		strcpy(pm2, dayBefore);
		strcat(pm2, " 14시");

		qna("강수량", frame.date[0], frame.loc[0]);
		strcpy(rainAmt, result_of_qna);

		qna("최고온도", pm, frame.loc[0]); // 오늘
		strcpy(hTemp1, result_of_qna);

		qna("최고온도", pm2, frame.loc[0]); // 어제
		strcpy(hTemp2, result_of_qna);

		printf("$하늘상태1:%s\n$하늘상태2:%s\n$강수량:%s\n$낮최고온도:%s,%s\n",
			skyStat1,skyStat2,rainAmt,hTemp1,hTemp2);
		fprintf(ff,"$하늘상태1:%s\n$하늘상태2:%s\n$강수량:%s\n$낮최고온도:%s,%s\n",
			skyStat1,skyStat2,rainAmt,hTemp1,hTemp2);

		if(strcmp(skyStat1, skyStat2) != 0 && strcmp(hTemp1, hTemp2) == 0){
			printf("%s %s은 하늘 상태는 '%s'에서 '%s'이며, 강수량은 '%s'이고, 낮 최고기온은 '%s'입니다\n",frame.date[0],frame.loc[0],skyStat1,skyStat2, rainAmt, hTemp1);
			fprintf(ff,"%s %s은 하늘 상태는 '%s'에서 '%s'이며, 강수량은 '%s'이고, 낮 최고기온은 '%s'입니다\n",frame.date[0],frame.loc[0],skyStat1,skyStat2, rainAmt, hTemp1);
		}else if(strcmp(skyStat1, skyStat2) == 0 && strcmp(hTemp1, hTemp2) == 0){
			printf("%s %s은 하늘 상태는 하루 종일 '%s'이며, 강수량은 '%s'이고, 낮 최고기온은 '%s'입니다\n",frame.date[0],frame.loc[0],skyStat1,rainAmt, hTemp1);
			fprintf(ff,"%s %s은 하늘 상태는 하루 종일 '%s'이며, 강수량은 '%s'이고, 낮 최고기온은 '%s'입니다\n",frame.date[0],frame.loc[0],skyStat1,rainAmt, hTemp1);
		}
	}
	fclose(ff);
	//fprintf();
}

void printAll(void) {
	int i;
	char tmp[MAX_WORD_SIZE];

	switch(spattern) {
	case -1:
		printf("매치되는 패턴이 없습니다.\n");
		break;
	case 10:
		printf("-명령어-전화-전화걸기\n");
		break;
	case 11:
		printf("-명령어-전화-전화끊기\n");
		break;
	case 20:
		printf("-명령어-타이머-타이머설정\n");
		break;
	case 21:
		printf("-명령어-타이머-타이머제어\n");
		break;
	case 31:
		printf("-명령어-문자-문자전송\n");
		break;
	case 32:
		printf("-명령어-문자-문자다수에게전송\n");
		break;
	case 33:
		printf("-명령어-문자-문자읽기\n");
		break;
	case 0:
		printf("-QnA-날씨-단순날씨요소질의\n");
		break;
		/* tkhwang */
	case 1:
		printf("-QnA-날씨-복합날씨요소질의\n");
		break;
	case 2:
		printf("-QnA-날씨-지역비교질의\n");
		break;
	case 3:
		printf("-QnA-날씨-날짜비교질의\n");
		break;
	case 4:
		printf("-QnA-날씨-날씨전반질의\n");
		break;
		/* tkhwang */
	}
	for(i = 0; i < count_of_pos; i++) {
		printf("%s, %c, %s, %s\n", pos_data[i].word, pos_data[i].chr, pos_data[i].type, pos_data[i].detail);
	}
	if(strcmp(frame.loc[0],"")==0)
		strcpy(frame.loc[0],"서울");

	if(strcmp(frame.date[0],"")==0) {
		time_t now = time(NULL);
		tm *gmtm = localtime(&now);
		char dt[BUFFER_SIZE];

		getNextDay(gmtm, 0);
		itoa(1900+gmtm->tm_year, frame.date[0], 10);
		strcat(frame.date[0], "년 ");
		itoa(gmtm->tm_mon+1, dt, 10);
		strcat(frame.date[0], dt);
		strcat(frame.date[0], "월 ");
		itoa(gmtm->tm_mday, dt, 10);
		strcat(frame.date[0], dt);
		strcat(frame.date[0], "일");
	}

	printf("==================================================================\n");
	switch(spattern) {
	case -1:
		printf("매치되는 응답프레임이 없습니다.\n");
		break;
	case 10:
		strcpy(frame.focus,"전화걸기");
		printf("focus:%s\n",frame.focus);
		printf("$person:%s\n",frame.person1);
		printf("%s에게 전화 걸겠습니다.\n",frame.person1);
		break;
	case 11:
		strcpy(frame.focus,"전화끊기");
		printf("focus:%s\n",frame.focus);
		printf("empty slot\n");
		printf("전화 끊겠습니다.\n");
		break;
	case 20:
		strcpy(frame.focus,"타이머설정");
		printf("focus:%s\n",frame.focus);
		printf("$time:%s\n",frame.time1);
		printf("타이머 %s 설정하였습니다.\n",frame.time1);
		break;
	case 21:
		strcpy(frame.focus,"타이머제어");
		printf("focus:%s\n",frame.focus);
		printf("$timeract:%s\n",frame.timeract);
		printf("타이머 %s 하였습니다.\n",frame.timeract);
		break;
	case 31:
		strcpy(frame.focus,"문자전송");
		printf("focus:%s\n",frame.focus);
		printf("$person:%s\n$content:%s\n",frame.person1,frame.content);
		printf("%s에게 %s라고 문자를 보냈습니다.\n",frame.person1,frame.content);
		break;
	case 32:
		strcpy(frame.focus,"문자다수에게전송");
		printf("focus:%s\n",frame.focus);
		printf("$person1:%s\n$person2:%s\n$content:%s\n",frame.person1,frame.person2,frame.content);
		printf("%s과 %s에게 %s라고 문자를 보냈습니다.\n",frame.person1,frame.person2,frame.content);
		break;
	case 33:
		strcpy(frame.focus,"문자읽기");
		printf("focus:%s\n",frame.focus);
		printf("$person:%s\n",frame.person1);
		printf("%s에게서 온 문자를 읽었습니다.\n",frame.person1);
		break;
	case 0:
		response("단순날씨구성요소","날씨요소_(",1);
		break;
	case 1:
		response("복합날씨구성요소","날씨요소(",2);
		break;
	case 2:
		response("단순날씨구성요소지역비교","날씨요소_지역비교(",2);
		break;
	case 3:
		response("단순날씨구성요소날짜비교","날씨요소_날짜비교(",2);
		break;
	case 4:
		response("종합날씨","날씨(",3);
		break;
	}
}

int cons(int src, int dest) {
	POS temp = {'\0', "", ""};
	int i =0;

	// 잘못 되었을 경우
	if(src >= count_of_pos || dest >= count_of_pos || src >= dest) {
		return 0;
	}

	strcat(pos_data[src].word, pos_data[dest].word);

	for(i = dest; i < count_of_pos; i++) {
		pos_data[i] = pos_data[i+1];
	}

	pos_data[count_of_pos-1] = temp;
	count_of_pos--;

	return 1;
}
void getNextDay(tm *gmtm, int numDay) {
	int month, year, day, maxDay = 0;
	time_t now = time(NULL);
	tm *today = localtime(&now);

	year = today->tm_year;
	month = today->tm_mon;
	day = today->tm_mday + numDay;

	if(day < 1) month--;

	if (month == 4 || month == 6 || month == 9 || month == 11) 	{
		maxDay = 30;
	} else if (month == 2) {
		bool isLeapYear = (year% 4 == 0 && year % 100 != 0) || (year % 400 == 0);
		if (isLeapYear) { 
			maxDay = 29;
		} else {
			maxDay = 28;
		}
	} else {
		maxDay = 31;
	}

	if(maxDay < day) {
		if(month == 12) {
			year++;
			month = 1;
		} else {
			month++;
		}
		day = (day-maxDay);
	} else if(day < 1) {
		if(month == 1) {
			year--;
			month = 12;
		} else {
			month--;
		}
		day = maxDay - day;
	}

	gmtm->tm_year = year;
	gmtm->tm_mon = month;
	gmtm->tm_mday = day;
}

int qna(char* component, char* date, char* location) {
	char tmp[BUFFER_SIZE], inputDate[BUFFER_SIZE] = "", *ptr;
	int i, j, num_of_work, inputDay = 1, inputHour = 14;
	time_t now = time(NULL);
	tm *today = localtime(&now);
	FILE *fp;

	strcpy(tmp, date);
	ptr = strtok(tmp, " ");
	num_of_work = 0;
	// char* YYYY년 MM월 DD일 TT시 -> char[] YYYY-MM-DD, int TT
	while(ptr != NULL) {
		substr(ptr, 0, strlen(ptr) - 2);
		switch(num_of_work) {
		case 0: 
			strcat(inputDate, ptr);
			break;
		case 1:
			strcat(inputDate, "-");
			strcat(inputDate, ptr);
			break;
		case 2:
			strcat(inputDate, "-");
			strcat(inputDate, ptr);
			inputDay = atoi(ptr) - today->tm_mday; // 오늘0 내일1 모레2
			break;
		case 3:
			inputHour = atoi(ptr);
			break;
		default:
			break;
		}
		num_of_work++;
		ptr = strtok(NULL, " ");
	}
	// 이 if문은 데이터베이스를 읽어오는 부분
	if(kma_data == NULL) {
		fp = fopen("kma.txt", "r");
		count_of_kma = 0;
		while(!feof(fp)) {
			fscanf(fp, "%s", tmp);
			count_of_kma++;	
		}
		kma_data = (KMA*)malloc(sizeof(KMA)*count_of_kma);
		fseek(fp, 0, SEEK_SET);
		// kma.txt 한 줄씩 돌아가면서
		for(i = 0; i < count_of_kma; i++) {
			fscanf(fp, "%s", tmp);
			ptr = strtok(tmp, "|");
			num_of_work = 0;
			while(ptr != NULL) {
				switch(num_of_work) {
				case 0:
					strcpy(kma_data[i].index, ptr);
					break;
				case 1:
					strcpy(kma_data[i].tm, ptr);
					break;
				case 2:
					strcpy(kma_data[i].city, ptr);
					break;
				case 3:
					kma_data[i].hour = atoi(ptr);
					break;
				case 4:
					strcpy(kma_data[i].day, ptr);
					break;
				case 5:
					strcpy(kma_data[i].temp, ptr);
					break;
				case 6:
					strcpy(kma_data[i].tmx, ptr);
					break;
				case 7:
					strcpy(kma_data[i].tmn, ptr);
					break;
				case 8:
					strcpy(kma_data[i].sky, ptr);
					break;
				case 9:
					strcpy(kma_data[i].pty, ptr);
					break;
				case 10:
					strcpy(kma_data[i].wfKor, ptr);
					break;
				case 11:
					strcpy(kma_data[i].wfEn, ptr);
					break;
				case 12:
					strcpy(kma_data[i].pop, ptr);
					break;
				case 13:
					strcpy(kma_data[i].r12, ptr);
					break;
				case 14:
					strcpy(kma_data[i].s12, ptr);
					break;
				case 15:
					strcpy(kma_data[i].ws, ptr);
					break;
				case 16:
					strcpy(kma_data[i].wd, ptr);
					break;
				case 17:
					strcpy(kma_data[i].wdKor, ptr);
					break;
				case 18:
					strcpy(kma_data[i].wdEn, ptr);
					break;
				case 19:
					strcpy(kma_data[i].reh, ptr);
					break;
				case 20:
					strcpy(kma_data[i].r06, ptr);
					break;
				case 21:
					strcpy(kma_data[i].s06, ptr);
					break;
				default:
					break;
				}
				ptr=strtok(NULL, "|");
				num_of_work++;
			}
		}
		fclose(fp);
	}
	// 이 if문은 데이터베이스를 읽어오는 부분
	if(kmw_data == NULL) {
		fp = fopen("kmw.txt", "r");
		count_of_kmw = 0;
		while(!feof(fp)) {
			fscanf(fp, "%s", tmp);
			count_of_kmw++;	
		}
		kmw_data = (KMW*)malloc(sizeof(KMW)*count_of_kmw);
		fseek(fp, 0, SEEK_SET);
		// kmw.txt 한 줄씩 돌아가면서
		for(i = 0; i < count_of_kmw; i++) {
			fscanf(fp, "%s", tmp);
			ptr = strtok(tmp, "|");
			num_of_work = 0;
			while(ptr != NULL) {
				switch(num_of_work) {
				case 0:
					strcpy(kmw_data[i].province, ptr);
					break;
				case 1:
					strcpy(kmw_data[i].city, ptr);
					break;
				case 2:
					strcpy(kmw_data[i].numEf, ptr);
					break;
				case 3:
					strcpy(kmw_data[i].tmEf, ptr);
					break;
				case 4:
					strcpy(kmw_data[i].wf, ptr);
					break;
				case 5:
					strcpy(kmw_data[i].tmn, ptr);
					break;
				case 6:
					strcpy(kmw_data[i].tmx, ptr);
					break;
				case 7:
					strcpy(kmw_data[i].reliability, ptr);
					break;
				default:
					break;
				}
				ptr=strtok(NULL, "|");
				num_of_work++;
			}
		}
		fclose(fp);
	}

	for(i = 0; i < count_of_kma; i++) {
		if(strcmp(kma_data[i].city, location) == 0 && strcmp(kma_data[i].day, inputDate) == 0) {
			if((kma_data[i].hour - inputHour) <= 1 && (kma_data[i].hour - inputHour) >= -1) {
				if((strcmp(component, "온도")) == 0) {
					strcpy(result_of_qna, kma_data[i].temp);
					strcat(result_of_qna, "℃");
					return 0;
				} else if((strcmp(component, "하늘상태")) == 0) {
					strcpy(result_of_qna, kma_data[i].wfKor);
					return 0;
				} else if((strcmp(component, "강수확률")) == 0) {
					strcpy(result_of_qna, kma_data[i].pop);
					strcat(result_of_qna, "%");
					return 0;
				} else if((strcmp(component, "강수상태")) == 0) {
					strcpy(result_of_qna, kma_data[i].pty);
					return 0;
				} else if((strcmp(component, "강수량")) == 0) {
					strcpy(result_of_qna, kma_data[i].r12);
					strcat(result_of_qna, "mm");
					return 0;
				} else if((strcmp(component, "강설량")) == 0) {
					strcpy(result_of_qna, kma_data[i].s12);
					strcat(result_of_qna, "cm");
					return 0;
				} else if((strcmp(component, "풍속")) == 0) {
					strcpy(result_of_qna, kma_data[i].ws);
					strcat(result_of_qna, "m/s");
					return 0;
				} else if((strcmp(component, "풍향")) == 0) {
					strcpy(result_of_qna, kma_data[i].wdKor);
					return 0;
				} else if((strcmp(component, "습도")) == 0) {
					strcpy(result_of_qna, kma_data[i].reh);
					strcat(result_of_qna, "%");
					return 0;
				} else if((strcmp(component, "최고온도")) == 0) {
					strcpy(result_of_qna, kma_data[i].tmx);
					strcat(result_of_qna, "℃");
					return 0;
				} else if((strcmp(component, "최저온도")) == 0) {
					strcpy(result_of_qna, kma_data[i].tmn);
					strcat(result_of_qna, "℃");
					return 0;
				}			 	
			}			
		}		
	}
	for(i = 0; i < count_of_kmw; i++) {
		if(strcmp(kmw_data[i].city, location) == 0) {
			if(strcmp(kmw_data[i].tmEf, inputDate)==0) {
				if((strcmp(component, "최고온도")) == 0) {
					strcpy(result_of_qna, kmw_data[i].tmx);
					strcat(result_of_qna, "℃");
					return 0;
				} else if((strcmp(component, "최저온도")) == 0) {
					strcpy(result_of_qna, kmw_data[i].tmn);
					strcat(result_of_qna, "℃");
					return 0;
				} else if((strcmp(component, "하늘상태")) == 0) {
					strcpy(result_of_qna, kmw_data[i].wf);
					return 0;
				} else if((strcmp(component, "신뢰도")) == 0) {
					strcpy(result_of_qna, kmw_data[i].reliability);
					return 0;
				}				
			}			
		}		
	}
	// 검색 값이 없을 경우
	return -1;
}