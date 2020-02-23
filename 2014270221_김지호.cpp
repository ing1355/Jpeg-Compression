#pragma warning(disable:4996)
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define WIDTH 512
#define HEIGHT 512

unsigned char buff[HEIGHT][WIDTH]; // 이미지를 담을 버퍼
double DCT[HEIGHT][WIDTH]; // 이미지 DCT 변환한 후에 담을 곳
double YangJaHwa[HEIGHT][WIDTH]; // 양자화 구현한 거 담을 곳
double ZigZag[64 * 64][64]; // 지그재그 구현한거 담을 곳
char *JPEG_Result[64 * 64]; // 압축 완료 비트스트림 담을 곳
char *JPEG_Result_temp; // DC계수와 AC계수를 합치기 전 DC계수를 담을 버퍼

char *SSS[11] = { "011","100","00","101","110","1110","11110","111110","1111110","11111110","111111110" }; //DC계수 검사 시 SSS에 따른 부호어

char *AC_Table[16][6] = { { "00","01","100","1011","11010","1111000" },
{ "1100","111001","1111001","111110110","11111110110","1111111110000100" },
{ "11011","11111000","1111100111","1111111110001001","1111111110001010","1111111110001011" },
{ "111010","111110111","11111110111","1111111110010000","1111111110010001","1111111110010010" },
{ "111011","1111111000","1111111110010111","1111111110011000","1111111110011001","1111111110011010" },
{ "1111010","1111111001","1111111110011111","1111111110100000","1111111110100001","1111111110100010" },
{ "1111011","11111111000","1111111110100111","1111111110101000","1111111110101001","1111111110101010" },
{ "11111001","11111111001","1111111110101111","1111111110110000","1111111110110001","1111111110110010" },
{ "1010","00","01","100","1011","11010" },
{ "1111111110000011","1100","111001","1111001","111110110","11111110110" },
{ "1111111110001000","11011","11111000","1111100111","1111111110001001","1111111110001010" },
{ "1111111110001111","111010","111110111","11111110111","1111111110010000","1111111110010001" },
{ "1111111010","1111111111011010","1111111111011011","1111111111011100","1111111111011101","1111111111011110" },
{ "11111111010","1111111111100011","1111111111100100","1111111111100101","1111111111100110","1111111111100111" },
{ "111111110110","1111111111101100","1111111111101101","1111111111101110","1111111111101111","1111111111110000" },
{ "11111111011","1111111111110101","1111111111110110","1111111111110111","1111111111111000","1111111111111001" } };
//AC 계수 부호어 테이블

double Quan_table[8][8] = { { 16, 11, 10, 16, 24, 40, 51, 61 },
{ 12, 12, 14, 19, 26, 58, 60, 55 },
{ 14, 13, 16, 24, 40, 57, 69, 56 },
{ 14, 17, 22, 29, 51, 87, 80, 62 },
{ 18, 22, 37, 56, 68, 109, 103, 77 },
{ 24, 35, 55, 64, 81, 104, 113, 92 },
{ 49, 64, 78 ,87, 103, 121, 120, 101 },
{ 72, 92, 95, 98, 112, 100, 103, 99 }
};

int Func_IDCT(int, int); // IDCT 함수

char* IntToStringBinaryDC(int); // DC계수 변환 함수
char* IntToStringBinaryAC(double*); // AC계수 변환 함수

int main() {
	int i, j, k, l, n, m;
	double cu, cv, result;
	FILE* fp1 = fopen("image.raw", "rb");
	fread(buff, sizeof(unsigned char), WIDTH*HEIGHT, fp1);
	fclose(fp1);

	for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) {
			for (k = 0; k < 8; k++) {
				for (l = 0; l < 8; l++) {
					result = 0;
					for (m = 0; m < 8; m++) {
						for (n = 0; n < 8; n++) {
							result += cos((((2 * (double)m + 1)*(double)k*M_PI) / 16))*cos((((2 * (double)n + 1)*(double)l*M_PI) / 16))*(double)buff[(i * 8) + m][(j * 8) + n];
						}
					}
					if (k == 0) cu = 1 / sqrt(2); else cu = 1;
					if (l == 0) cv = 1 / sqrt(2); else cv = 1;
					DCT[(i * 8) + k][(j * 8) + l] = result*((cu*cv) / 4);
				}
			}
		}
	}// DCT
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) {
			for (k = 0; k < 8; k++) {
				for (l = 0; l < 8; l++) {
					YangJaHwa[(i * 8) + k][(j * 8) + l] = floor((DCT[(i * 8) + k][(j * 8) + l] / Quan_table[k][l]) + 0.5);
				}
			}
		}
	}// 양자화

	int r = 0, c = 0, r_next = 0, c_next = 0;
	int Controll = 1;
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) {
			r = 0;
			c = 0;
			for (k = 0; k < 8; k++) {
				for (l = 0; l < 8; l++) {
					ZigZag[(i * 8) + j][k * 8 + l] = YangJaHwa[(i * 8) + r][(j * 8) + c];
					if (Controll == 1) {
						r_next = r - 1;
						c_next = c + 1;
					}
					else if (Controll == 2) {
						r_next = r + 1;
						c_next = c - 1;
					}
					if (r_next < 0 || c_next < 0 || r_next >= 8 || c_next >= 8) {
						if (r_next < 0 && c_next >= 8) r += 1;
						else if (r_next >= 8 && c_next < 0) c += 1;
						else if (r_next < 0) c += 1;
						else if (c_next < 0) r += 1;
						else if (r_next >= 8) c += 1;
						else if (c_next >= 8) r += 1;
						Controll = (Controll == 1) ? 2 : 1;
					}
					else {
						r = r_next; c = c_next;
					}
				}
			}
		}
	} // 지그재그 구현
		for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) {
		if ((j==0) && (i == 0)) {
		JPEG_Result[0] = (char*)malloc(strlen(IntToStringBinaryDC((int)ZigZag[0][0]) + 1));
		strcpy(JPEG_Result[0], IntToStringBinaryDC((int)ZigZag[0][0]));
		}
		else {
		JPEG_Result[(i * 8) + j] = (char*)malloc(strlen(IntToStringBinaryDC((int)(ZigZag[(i * 64) + j][0] - ZigZag[((i * 64) + j)-1][0]))) + 1);
		strcpy(JPEG_Result[(i * 8) + j], IntToStringBinaryDC((int)(ZigZag[(i*64)+j][0]- ZigZag[((i * 64) + j)-1][0])));
		}
		//JPEG_Result[(i * 8) + j] = (char*)malloc(strlen(IntToStringBinaryDC(YangJaHwa[(i * 8)][j * 8])) + 1);
		//strcpy(JPEG_Result[(i * 8) + j], IntToStringBinaryDC(YangJaHwa[(i * 8)][j * 8]));
		JPEG_Result_temp = (char*)malloc(strlen(JPEG_Result[(i * 8) + j])+1);
		strcpy(JPEG_Result_temp, JPEG_Result[(i*8)+j]);
		JPEG_Result[(i * 8) + j] = (char*)realloc(JPEG_Result[(i * 8) + j], strlen(JPEG_Result[(i * 8) + j]) + strlen(IntToStringBinaryAC(ZigZag[(i * 8) + j]) + 1));
		strcpy(JPEG_Result[(i*8)+j], JPEG_Result_temp);
		strcat(JPEG_Result[(i * 8) + j], IntToStringBinaryAC(ZigZag[(i * 8) + j]));
		}
		} // 양자화한 것에서 DC계수와 AC계수를 이어붙여 결과값을 저장할 곳에 넣는다.
		for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) {
		printf("%s\n", JPEG_Result[i * 8 + j]);
		}
		}

	return 0;
}

char* IntToStringBinaryDC(int N) {
	char *arr, *arr2;
	int X = 1, Y = 2;
	while (Y < abs(N)) {
		Y *= 2;
		X++;
	} // 범주 찾기
	if (N == 0) {
		arr = (char*)malloc(5);
		strcpy(arr, "010 ");
		arr2 = (char*)malloc(1);
		free(arr2);
		return arr;
	} // DC계수가 0일때
	else {
		arr = (char*)malloc(X + 2);
		itoa(abs(N), arr, 2);
		if (N > 0) {
			arr2 = (char*)malloc(strlen(SSS[X - 1]) + strlen(arr) + 2);
			strcpy(arr2, SSS[X - 1]);
			strcat(arr2, arr);
			strcat(arr2, " ");
			free(arr);
			return arr2;
		} // 양수면 2진변환한 것 그대로.
		else if (N < 0) {
			int i = 0;
			while (arr[i] != '\0') {
				if (arr[i] == '0')
					arr[i] = '1';
				else
					arr[i] = '0';
				i++;
			}
			arr2 = (char*)malloc(strlen(SSS[X - 1]) + strlen(arr) + 2);
			strcpy(arr2, SSS[X - 1]);
			strcat(arr2, arr);
			strcat(arr2, " ");
			free(arr);
			return arr2;
		} // 음수면 1의 보수 취해서
	}
}
char* IntToStringBinaryAC(double *N) {
	char *arr, *arr2, *arr3, *arr_temp;
	int Zero_Length = 0, count_X;
	int X, Y;
	arr = (char*)malloc(1); // 최종적 8x8 배열의 비트스트림
	arr_temp = (char*)malloc(1);
	strcpy(arr, " ");
	strcpy(arr_temp, " ");
	arr2 = (char*)malloc(28); // 각 AC계수들의 (0 Length, 범주)의 부호어 + 후에 합칠 곳.
	arr3 = (char*)malloc(12); // 각 AC계수들의 (0 Length, Value)의 부호어
	for (int i = 1; i < 64; i++) {
		count_X = 0;
		X = 1;
		Y = 2;
		if (N[i] == 0) {
			Zero_Length++; // 0의 길이 증가
			if (Zero_Length == 16) {
				for (int j = i; j < 64; j++) {
					if (N[j] != 0) {
						count_X = 1;
						break;
					} // 0이 15개가 나와도 뒤에 실값이 있으면 0이 15개의 부호어로 적용
				}
				if (count_X == 1) {
					strcpy(arr2, "11111111011 ");
					arr_temp = (char*)realloc(arr_temp, strlen(arr) + 1);
					strcpy(arr_temp, arr);
					arr = (char*)realloc(arr, strlen(arr_temp) + strlen(arr2) + 2);
					strcpy(arr, arr_temp);
					strcat(arr, arr2);
				} //0이 15개가 최대라고 했을 때 뒤에 실값이 있으면 EOB가 아닌 Run_length 15 기준 부호어
				else {
					strcpy(arr2, "1010 ");
					arr_temp = (char*)realloc(arr_temp, strlen(arr) + 1);
					strcpy(arr_temp, arr);
					arr = (char*)realloc(arr, strlen(arr_temp) + strlen(arr2) + 2);
					strcpy(arr, arr_temp);
					strcat(arr, arr2);
					free(arr2);
					free(arr3);
					free(arr_temp);
					return arr;
				}// EOB일 때 부호어
				Zero_Length = 1;
			}
		}
		else {
			itoa(abs((int)N[i]), arr3, 2); // Value 값 2진 변환
			while (Y < abs((int)N[i])) {
				Y *= 2;
				X++;
			}
			// Value에 따른 범주 값 계산
			switch (X) {
			case 1:
				strcpy(arr2, AC_Table[Zero_Length][0]);
				break;
			case 2:
				strcpy(arr2, AC_Table[Zero_Length][1]);
				break;
			case 3:
				strcpy(arr2, AC_Table[Zero_Length][2]);
				break;
			case 4:
				strcpy(arr2, AC_Table[Zero_Length][3]);
				break;
			case 5:
				strcpy(arr2, AC_Table[Zero_Length][4]);
				break;
			case 6:
				strcpy(arr2, AC_Table[Zero_Length][5]);
				break;
			} // 0 길이와 SIZE에 대조되는 AC_Table 입력
			arr_temp = (char*)realloc(arr_temp, strlen(arr));
			strcpy(arr_temp, arr);
			arr = (char*)realloc(arr, strlen(arr) + strlen(arr2) + strlen(arr3) + 1);
			strcpy(arr, arr_temp);
			if (N[i] > 0) {
				strcat(arr, arr2);
				strcat(arr, arr3);
				strcat(arr, " ");
			}
			// Value의 부호어 + 구분을 위한 빈칸
			else {
				int tmp = 0;
				while (arr3[tmp] != '\0') {
					if (arr3[tmp] == '0')
						arr3[tmp] = '1';
					else
						arr3[tmp] = '0';
					tmp++;
				} // 음수일 시 1의 보수 취함
				strcat(arr, arr2);
				strcat(arr, arr3);
				strcat(arr, " ");
				//Value의 부호어 + 구분을 위한 빈칸
			}
			Zero_Length = 0;
		}
	}
	free(arr2);
	free(arr3);
	free(arr_temp);
	return arr;
}


int Func_IDCT(int x, int y) {
double result = 0;
double C_u, C_v;
for (int i = 0; i < 8; i++) {
for (int j = 0; j < 8; j++) {
if (i == 0)
C_u = 1 / sqrt(2);
else
C_u = 1;
if (j == 0)
C_v = 1 / sqrt(2);
else
C_v = 1;
result += ((C_u*C_v) / 4) * DCT[i][j] * cos((((2 * x + 1) *i) *M_PI) / 16) * cos((((2 * y + 1) *j) *M_PI) / 16);
}
}
return result;
}