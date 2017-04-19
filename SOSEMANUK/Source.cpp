#include <iostream>
#include <CL/cl.h>
#include <iomanip>
#include <sstream>
#include <fstream>

using namespace std;
const char* Sosemanuk_Kernel;

string kernel;

const int ROZMIAR_KLUCZA = 256;

const int ROZMIAR_VEKTORA_KLUCZY = 1024;

size_t ILOSC_INTOW_NA_RAZ = 20000;  // musi być podziele przez 4

size_t SIZE_KERNEL_SERPENT = 0;

string normalizuj_klucz(string key)
{
	if (key.length() % 2 == 1) key += "0";
	string key_temp = "";

	for (size_t i = 0; i < key.length(); i += 2) {
		key_temp = key.substr(i, 1) + key.substr(i + 1, 1) + key_temp;
	}

	key_temp = "1" + key_temp;
	while ((key_temp.length() * 4) < ROZMIAR_KLUCZA) {
		key_temp = "0" + key_temp;
	}

	return key_temp;

}

string intToHexString(int intValue) {

	string hexStr;

	stringstream sstream;
	sstream << setfill('0') << setw(2) << hex << (int)intValue;

	hexStr = sstream.str();
	sstream.clear();

	return hexStr;
}

unsigned int LittleEndian(unsigned int val)
{

	unsigned int a = (val & 0xFF000000) >> 24;
	unsigned int b = (val & 0xFF0000) >> 8;
	unsigned int c = (val & 0xFF00) << 8;
	unsigned int d = (val & 0xFF) << 24;

	return a ^ b ^ c ^ d;
}

cl_int readFile(const char *filename, string& s)
{
	size_t size;
	char*  str;
	fstream f(filename, (fstream::in | fstream::binary));

	if (f.is_open())
	{
		size_t fileSize;
		f.seekg(0, fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, fstream::beg);
		str = new char[size + 1];
		if (!str)
		{
			f.close();
			return 0;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return 0;
	}
	cout << "BLAD ODCZYTU KERNELA: \n:" << filename << endl;
	return 1;
}


void read_kernal() 
{
	fstream plik;

	readFile("Kernel.c", kernel);

	Sosemanuk_Kernel = kernel.c_str() ;
	SIZE_KERNEL_SERPENT = kernel.length();

}


char swap(char c)
{
	switch(c)
	{
		case'0': return '1';
		case'1': return '2';
		case'2': return '3';
		case'3': return '4';
		case'4': return '5';
		case'5': return '6';
		case'6': return '7';
		case'7': return '8';
		case'8': return '9';
		case'9': return 'A';
		case'A': return 'B';
		case'B': return 'C';
		case'C': return 'D';
		case'D': return 'E';
		case'E': return 'F';
		case'F': return '0';
	}
}

void dodaj(string &klucz, int dlugosc)
{
	if (dlugosc == 0) return ;
	
	klucz[dlugosc - 1] = swap(klucz[dlugosc - 1]);

	if (klucz[dlugosc - 1] == '0') dodaj(klucz, dlugosc - 1);
}

void zapisz_do_pliku(string sciezka, unsigned int *output_stream, int ILOSC_WEKTOROW, int ILOSC_INTOW_NA_RAZ, int seria, int paczka, bool czy_ostatnie, size_t DLUGOSC_STRUMIENIA_KLUCZA_BAJTY, string *klucz_str, string *IV_str)
{
	for (size_t j = 0; j < ILOSC_WEKTOROW; j++)
	{
		 
		ostringstream ss;
		ss << j + paczka * ROZMIAR_VEKTORA_KLUCZY;

		string path = sciezka + ss.str();
		fstream plik(path, ios::app);

		int dlugosc_podserii = ILOSC_INTOW_NA_RAZ / 4;
		
		/*wyjątek dla równości*/
		if (DLUGOSC_STRUMIENIA_KLUCZA_BAJTY % (ILOSC_INTOW_NA_RAZ * 4) == 0)
		{
			czy_ostatnie = false;
		}

		if(czy_ostatnie)
		{
			int temp = (ILOSC_INTOW_NA_RAZ / 4) * (DLUGOSC_STRUMIENIA_KLUCZA_BAJTY % (ILOSC_INTOW_NA_RAZ * 4));
			dlugosc_podserii = temp / (ILOSC_INTOW_NA_RAZ * 4);

			if(temp % (ILOSC_INTOW_NA_RAZ * 4) != 0)
			{
				dlugosc_podserii++;
			}

		}

		if (plik.good())
		{
			if (seria == 0)
			{

				plik << klucz_str[j] << "  " << endl;
				plik << IV_str[j] << " " << endl;
			}
			for (size_t podseria = 0; podseria < dlugosc_podserii; podseria++)
			{
				int ilosc = 4;
				bool czy_ostatnia_podseria = false;

				if (czy_ostatnie && podseria == dlugosc_podserii - 1)
				{
					ilosc = (DLUGOSC_STRUMIENIA_KLUCZA_BAJTY % (4 * 4)) / 4;

					if((DLUGOSC_STRUMIENIA_KLUCZA_BAJTY % (4 * 4)) % 4 != 0)
					{
						ilosc++;
					}

					czy_ostatnia_podseria = true;
				}

				for (size_t k = 0; k < ilosc; k++) {

					ostringstream s_output;
					s_output << hex << uppercase << output_stream[podseria * (4 * ILOSC_WEKTOROW) + 4 * j + k];

					string str_temp = s_output.str();

					while (str_temp.length() < 8)
					{
						str_temp = '0' + str_temp;
					}
						
					if(czy_ostatnia_podseria && k == ilosc - 1 && DLUGOSC_STRUMIENIA_KLUCZA_BAJTY % 4 != 0)
					{
						int ilosc_bajtow_ostatniego = DLUGOSC_STRUMIENIA_KLUCZA_BAJTY % 4 * 2;
						plik << str_temp.substr(0, ilosc_bajtow_ostatniego);
					}
					else
					{
						plik << str_temp;
					}
				}
			}

			if (!czy_ostatnie && DLUGOSC_STRUMIENIA_KLUCZA_BAJTY % (ILOSC_INTOW_NA_RAZ * 4) != 0)
			{
				plik << endl;
			}

			plik.close();
		}
		else
		{
			cout << "ERROR nie otworzono pliku wyjsciowego !!! " << endl;
		}
	}
}

void sacl_pliki(string sciezka, int ILOSC_WEKTOROW, int paczka)
{
	fstream plik(sciezka, ios::out);
	
	if (plik.good())
	{
		for (size_t j = 0; j < ILOSC_WEKTOROW + paczka * ROZMIAR_VEKTORA_KLUCZY; j++)
		{
			ostringstream ss;
			ss << j;

			string path = sciezka + ss.str();
			fstream plik_in(path, ios::in);

			if (plik_in.good())
			{
				string stream;
				
				int temp_klucz_iv = 0;

				while(!plik_in.eof())
				{
					plik_in >> stream;

					plik << stream;

					if (temp_klucz_iv++ < 2)
					{
						plik << " ";
					}
				}

				if(j != ILOSC_WEKTOROW + paczka * ROZMIAR_VEKTORA_KLUCZY - 1)
				{
					plik << endl;
				}

				
				plik_in.close();
				remove(path.c_str());
			}
		}
		plik.close();
	}
}

int main(int argc, char **argv)
{
	system("cls");
	if (argc >= 7)
	{
		//Get an OpenCL platform
		cl_platform_id cpPlatform;

		clGetPlatformIDs(1, &cpPlatform, NULL);

		// Get a GPU device
		cl_device_id cdDevice;

		clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &cdDevice, NULL);

		cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)cpPlatform, 0 };

		// Create a context to run OpenCL enabled GPU
		cl_context GPUContext = clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, NULL, NULL, NULL);


		// Create a command-queue on the GPU device
		cl_command_queue cqCommandQueue = clCreateCommandQueue(GPUContext, cdDevice, 0, NULL);

		size_t dlugosc_klucza = atoi(argv[1]);
		size_t ILOSC_WEKTOROW = 0; //////////////
		string klucze = argv[2];
		string k_od = "";
		string k_do = "";

		if (klucze.find('-') == -1)
		{
			k_od = klucze;
			k_do = klucze;
		}
		else
		{
			k_od = klucze.substr(0, klucze.find('-'));
			k_do = klucze.substr(klucze.find('-') + 1);
		}

		if (k_od.find('x') != -1)
		{
			k_od = k_od.substr(2);
		}
		if (k_do.find('x') != -1)
		{
			k_do = k_do.substr(2);
		}

		while (k_od.length() < 10) k_od = "0" + k_od;
		while (k_do.length() < 10) k_do = "0" + k_do;

		size_t dlugosc_iv = stoi(argv[3]);

		string vektory_iv = argv[4];
		string stream_path = argv[5];
		size_t DLUGOSC_STRUMIENIA_KLUCZA_BAJTY = stoi(argv[6]);


		string iv_od = "";
		string iv_do = "";

		if (klucze.find('-') == -1)
		{
			iv_od = vektory_iv;
			iv_do = vektory_iv;
		}
		else
		{
			iv_od = vektory_iv.substr(0, vektory_iv.find('-'));
			iv_do = vektory_iv.substr(vektory_iv.find('-') + 1);
		}

		if (iv_od.find('x') != -1)
		{
			iv_od = iv_od.substr(2);
		}
		if (iv_do.find('x') != -1)
		{
			iv_do = iv_do.substr(2);
		}

		while (iv_od.length() < 32) iv_od = "0" + iv_od;
		while (iv_do.length() < 32) iv_do = "0" + iv_do;



		string *klucz_str = new string[ROZMIAR_VEKTORA_KLUCZY];

		// przykład
	//	klucz_str[0] = "A7C083FEB7";
	//	klucz_str[1] = "A7C083FEB8";
	//	klucz_str[2] = "A7C083FEB9";


		string *IV_str = new string[ROZMIAR_VEKTORA_KLUCZY];
		// przykład
	//	IV_str[0] = "00112233445566778899AABBCCDDEEFF";
	//	IV_str[1] = "00112233445566778899AABBCCDDEEFF";
	//	IV_str[2] = "00112233445566778899AABBCCDDEEFF";
		
		string klucz_temp = k_od;
		string iv_temp = iv_od;

		int paczka = 0;

		do
		{



			int licznik = 0;

			bool koniec_klucz = false;
			bool koniec_iv = false;

			bool flaga_startu = false;
			do
			{
				if (klucz_temp == k_do)
				{
					koniec_klucz = true;
				}

				koniec_iv = false;

				if (flaga_startu) {
					iv_temp = iv_od;
				}
				else
				{
					flaga_startu = true;
				}

				do
				{
					if (iv_temp == iv_do)
					{
						koniec_iv = true;
					}

					klucz_str[licznik] = klucz_temp;
					IV_str[licznik] = iv_temp;
					licznik++;
					
					dodaj(iv_temp, iv_temp.length());
					

				} while (!koniec_iv && licznik < ROZMIAR_VEKTORA_KLUCZY);

				if (licznik < ROZMIAR_VEKTORA_KLUCZY)
				{
					dodaj(klucz_temp, klucz_temp.length());
				}

			} while (!koniec_klucz && licznik < ROZMIAR_VEKTORA_KLUCZY);

			ILOSC_WEKTOROW = licznik;
			//cout << "ILOSC WEKTOROW  " << ILOSC_WEKTOROW << endl;

			unsigned int *klucz = new unsigned int[8 * ILOSC_WEKTOROW];

			string *klucz_str_plik = new string[ROZMIAR_VEKTORA_KLUCZY];

			for (size_t j = 0; j < ILOSC_WEKTOROW; j++) {

				klucz_str_plik[j] = klucz_str[j];
				
				klucz_str[j] = normalizuj_klucz(klucz_str[j]);

				for (size_t i = 0; i < 8; i++) {
					klucz[8 * j + i] = strtoul(klucz_str[j].substr(i * 8, 8).c_str(), NULL, 16);
				}
			}

			unsigned int *IV = new unsigned int[4 * ILOSC_WEKTOROW];

			// normalizowanie IV
			for (size_t j = 0; j < ILOSC_WEKTOROW; j++) {
				for (size_t i = 0; i < 4; i++)
				{
					IV[4 * j + 3 - i] = LittleEndian(strtoul(IV_str[j].substr(i * 8, 8).c_str(), NULL, 16));
				}
			}

			unsigned int *output = new unsigned int[12 * ILOSC_WEKTOROW];

			read_kernal();


			cl_int error;
			cl_program OpenCLProgram = clCreateProgramWithSource(GPUContext, 1, &Sosemanuk_Kernel, &SIZE_KERNEL_SERPENT, &error);

			// Build the program (OpenCL JIT compilation)
			error = clBuildProgram(OpenCLProgram, 1, &cdDevice, NULL, NULL, NULL);

			if (error != CL_SUCCESS) {
				size_t log_size = 0;
				clGetProgramBuildInfo(OpenCLProgram, cdDevice, CL_PROGRAM_BUILD_LOG, NULL, NULL, &log_size);
				char* log = (char*)malloc(sizeof(char) * log_size);
				clGetProgramBuildInfo(OpenCLProgram, cdDevice, CL_PROGRAM_BUILD_LOG, log_size, log, &log_size);
				cout << log;
				free(log);
			}

			// KEY SCHEDULE && IV 
			{
				cl_mem GPU_klucz = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(unsigned int) * 8 * ILOSC_WEKTOROW, klucz, NULL);
				cl_mem GPU_iv = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(unsigned int) * 4 * ILOSC_WEKTOROW, IV, NULL);
				cl_mem GPU_ILOSC_WEKTOROW = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(size_t), &ILOSC_WEKTOROW, NULL);

				cl_mem GPU_output = clCreateBuffer(GPUContext, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * 12 * ILOSC_WEKTOROW, NULL, NULL);

				// Create a handle to the compiled OpenCL function (Kernel)
				cl_kernel OpenCLVectorAdd = clCreateKernel(OpenCLProgram, "Key_schedule_IV", NULL);

				// In the next step we associate the GPU memory with the Kernel arguments
				error = clSetKernelArg(OpenCLVectorAdd, 0, sizeof(cl_mem), (void*)&GPU_klucz);
				error = clSetKernelArg(OpenCLVectorAdd, 1, sizeof(cl_mem), (void*)&GPU_iv);
				error = clSetKernelArg(OpenCLVectorAdd, 2, sizeof(cl_mem), (void*)&GPU_output);
				error = clSetKernelArg(OpenCLVectorAdd, 3, sizeof(cl_mem), (void*)&GPU_ILOSC_WEKTOROW);

				// This kernel only uses global data
				size_t WorkSize[1] = { ROZMIAR_VEKTORA_KLUCZY }; // one dimensional Range

				error = clEnqueueNDRangeKernel(cqCommandQueue, OpenCLVectorAdd, 1, NULL, WorkSize, NULL, 0, NULL, NULL);

				// Copy the output in GPU memory back to CPU memory
				error = clEnqueueReadBuffer(cqCommandQueue, GPU_output, CL_TRUE, 0, sizeof(unsigned int) * 12 * ILOSC_WEKTOROW, output, 0, NULL, NULL);

				error = 0;
			}

			// to powinno byc parametryzowane bo zostanie wygenerowane `n` wejsc 
			unsigned int *lsfr = new unsigned int[10 * ILOSC_WEKTOROW];
			unsigned int *fsmR = new unsigned int[2 * ILOSC_WEKTOROW];

			for (size_t i = 0; i < ILOSC_WEKTOROW; i++) {
				lsfr[10 * i + 0] = output[12 * i + 11];
				lsfr[10 * i + 1] = output[12 * i + 10];
				lsfr[10 * i + 2] = output[12 * i + 9];
				lsfr[10 * i + 3] = output[12 * i + 8];
				lsfr[10 * i + 4] = output[12 * i + 5];
				lsfr[10 * i + 5] = output[12 * i + 7];
				lsfr[10 * i + 6] = output[12 * i + 3];
				lsfr[10 * i + 7] = output[12 * i + 2];
				lsfr[10 * i + 8] = output[12 * i + 1];
				lsfr[10 * i + 9] = output[12 * i + 0];

				fsmR[2 * i + 0] = output[12 * i + 4];
				fsmR[2 * i + 1] = output[12 * i + 6];

			}

			delete[] output;

			// Generacja strumienia klucza
			int dlugosc_temp = DLUGOSC_STRUMIENIA_KLUCZA_BAJTY / (ILOSC_INTOW_NA_RAZ * 4);
			
			if(DLUGOSC_STRUMIENIA_KLUCZA_BAJTY % (ILOSC_INTOW_NA_RAZ * 4) != 0)
			{
				dlugosc_temp++;
			}

			for (size_t seria = 0; seria < dlugosc_temp; seria++)
			{
				// Allocate GPU memory for source vectors AND initialize from CPU memory
				cl_mem GPU_lfsr = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(unsigned int) * 10 * ILOSC_WEKTOROW, lsfr, NULL);
				cl_mem GPU_fsmR = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(unsigned int) * 2 * ILOSC_WEKTOROW, fsmR, NULL);
				cl_mem GPU_ILOSC_INTOW_NA_RAZ = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(unsigned int), &ILOSC_INTOW_NA_RAZ, NULL);
				cl_mem GPU_ILOSC_WEKTOROW = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(unsigned int), &ILOSC_WEKTOROW, NULL);

				// Allocate output memory on GPU
				cl_mem GPU_output_stream = clCreateBuffer(GPUContext, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * ILOSC_WEKTOROW * ILOSC_INTOW_NA_RAZ, NULL, NULL);
				cl_mem GPU_output_lfsr = clCreateBuffer(GPUContext, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * ILOSC_WEKTOROW * 10, NULL, NULL);
				cl_mem GPU_output_fsmR = clCreateBuffer(GPUContext, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * ILOSC_WEKTOROW * 2, NULL, NULL);
				
				// Create a handle to the compiled OpenCL function (Kernel)
				cl_kernel OpenCLVectorAdd = clCreateKernel(OpenCLProgram, "Make_stream", NULL);

				// In the next step we associate the GPU memory with the Kernel arguments
				error = clSetKernelArg(OpenCLVectorAdd, 0, sizeof(cl_mem), (void*)&GPU_lfsr);                //cout << error << endl;
				error = clSetKernelArg(OpenCLVectorAdd, 1, sizeof(cl_mem), (void*)&GPU_fsmR);                //cout << error << endl;
				error = clSetKernelArg(OpenCLVectorAdd, 2, sizeof(cl_mem), (void*)&GPU_ILOSC_INTOW_NA_RAZ);  //cout << error << endl;
				error = clSetKernelArg(OpenCLVectorAdd, 3, sizeof(cl_mem), (void*)&GPU_ILOSC_WEKTOROW);      //cout << error << endl;
				error = clSetKernelArg(OpenCLVectorAdd, 4, sizeof(cl_mem), (void*)&GPU_output_stream);       //cout << error << endl;
				error = clSetKernelArg(OpenCLVectorAdd, 5, sizeof(cl_mem), (void*)&GPU_output_lfsr);         //cout << error << endl;
				error = clSetKernelArg(OpenCLVectorAdd, 6, sizeof(cl_mem), (void*)&GPU_output_fsmR);         //cout << error << endl;

				// This kernel only uses global data
				size_t WorkSize[1] = { ROZMIAR_VEKTORA_KLUCZY }; // one dimensional Range

				error = clEnqueueNDRangeKernel(cqCommandQueue, OpenCLVectorAdd, 1, NULL, WorkSize, NULL, 0, NULL, NULL); //cout << error << endl;

				unsigned int *output_stream = new unsigned int[ILOSC_WEKTOROW * ILOSC_INTOW_NA_RAZ];

				// Copy the output in GPU memory back to CPU memory
				error = clEnqueueReadBuffer(cqCommandQueue, GPU_output_stream, CL_TRUE, 0, sizeof(unsigned int) * ILOSC_WEKTOROW * ILOSC_INTOW_NA_RAZ, output_stream, 0, NULL, NULL); //cout << error << endl;
				error = clEnqueueReadBuffer(cqCommandQueue, GPU_output_lfsr, CL_TRUE, 0, sizeof(unsigned int) * ILOSC_WEKTOROW * 10, lsfr, 0, NULL, NULL); //cout << error << endl;
				error = clEnqueueReadBuffer(cqCommandQueue, GPU_output_fsmR, CL_TRUE, 0, sizeof(unsigned int) * ILOSC_WEKTOROW * 2, fsmR, 0, NULL, NULL); //cout << error << endl;

				bool czy_ostatnie = (seria == dlugosc_temp - 1);

				zapisz_do_pliku(stream_path, output_stream, ILOSC_WEKTOROW, ILOSC_INTOW_NA_RAZ, seria, paczka, czy_ostatnie, DLUGOSC_STRUMIENIA_KLUCZA_BAJTY, klucz_str_plik, IV_str);

				//delete GPU_lfsr;
				//delete GPU_fsmR;
				//delete GPU_ILOSC_INTOW_NA_RAZ;
				//delete GPU_ILOSC_WEKTOROW;
				//
				//delete GPU_output_stream;
				//delete GPU_output_lfsr;
				//delete GPU_output_fsmR;
				//
				//delete OpenCLVectorAdd;


				delete[](output_stream);
			}

			delete[](klucz);
			delete[](IV);

			delete[](lsfr);
			delete[](fsmR);
			delete[](klucz_str_plik);
			paczka++;
		}while(ILOSC_WEKTOROW == ROZMIAR_VEKTORA_KLUCZY || (false));


		
		sacl_pliki(stream_path, ILOSC_WEKTOROW, paczka - 1);

	}

	return 0;

}