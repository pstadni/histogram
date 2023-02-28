//POP_2020_01_13_projekt_2_Stadnik_Paulina_IBM_2_180156 Code Blocks 17.12
#include <fstream>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <stdint.h>
#include <string>
#include <windows.h>

using namespace std;

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t typ_pliku{ 0x4D42 };
    uint32_t rozmiar_pliku{ 0 };
    uint16_t reserved1{ 0 };
    uint16_t reserved2{ 0 };
    uint32_t offset_data{ 0 };
};

struct BMPInfoHeader {
    uint32_t rozmiar{ 0 };
    int32_t szerokosc{ 0 };
    int32_t wysokosc{ 0 };
    uint16_t plaszczyzna{ 1 };
    uint16_t bit_count{ 0 };
    uint32_t compression{ 0 };
    uint32_t rozmiar_obrazka{ 0 };
    int32_t x_pixeli_na_metr{ 0 };
    int32_t y_pixeli_na_metr{ 0 };
    uint32_t colors_used{ 0 };
    uint32_t colors_important{ 0 };
};

struct BMPColorHeader {
    uint32_t R_mask{ 0x00ff0000 };
    uint32_t G_mask{ 0x0000ff00 };
    uint32_t B_mask{ 0x000000ff };
    uint32_t alpha_mask{ 0xff000000 };
    uint32_t color_space_type{ 0x73524742 };
    uint32_t unused{ 0 };
};

#pragma pack(pop)

struct BMP {
    BMPFileHeader naglowek_pliku;
    BMPInfoHeader naglowek_DIB;
    BMPColorHeader tablica_kolorow;
    vector<uint8_t> data;

    BMP() {}
    int read(const char* fname) {
        ifstream inp{ fname, ios_base::binary };
        if (inp) {
            inp.read((char*)& naglowek_pliku, sizeof(naglowek_pliku));
            if (naglowek_pliku.typ_pliku != 0x4D42) {
                cout << "To nie jest plik BMP" << endl;
            }
            inp.read((char*)& naglowek_DIB, sizeof(naglowek_DIB));


            if (naglowek_DIB.bit_count == 32) {

                if (naglowek_DIB.rozmiar >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))) {
                    inp.read((char*)& tablica_kolorow, sizeof(tablica_kolorow));

                    sprawdz_color_header(tablica_kolorow);
                }
                else {
                    cout << "Błąd\"" << fname << "\" nie zawiera informacji o masce bitowej\n";
                }
            }


            inp.seekg(naglowek_pliku.offset_data, inp.beg);

            if (naglowek_DIB.bit_count == 32) {
                naglowek_DIB.rozmiar = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
                naglowek_pliku.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            }
            else {
                naglowek_DIB.rozmiar = sizeof(BMPInfoHeader);
                naglowek_pliku.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
            }
            naglowek_pliku.rozmiar_pliku = naglowek_pliku.offset_data;

            if (naglowek_DIB.wysokosc < 0) {
                cout << "The program can treat only BMP images with the origin in the bottom left corner!" << endl;
            }

            data.resize(naglowek_DIB.szerokosc * naglowek_DIB.wysokosc * naglowek_DIB.bit_count / 8);


            if (naglowek_DIB.szerokosc % 4 == 0) {
                inp.read((char*)data.data(), data.size());
                naglowek_pliku.rozmiar_pliku += static_cast<uint32_t>(data.size());
            }
            else {
                row_stride = naglowek_DIB.szerokosc * naglowek_DIB.bit_count / 8;
                uint32_t new_stride = make_stride_aligned(4);
                vector<uint8_t> padding_row(new_stride - row_stride);

                for (int y = 0; y < naglowek_DIB.wysokosc; ++y) {
                    inp.read((char*)(data.data() + row_stride * y), row_stride);
                    inp.read((char*)padding_row.data(), padding_row.size());
                }
                naglowek_pliku.rozmiar_pliku += static_cast<uint32_t>(data.size()) + naglowek_DIB.wysokosc * static_cast<uint32_t>(padding_row.size());
            }
            return 1;
        }
        else {
            cout << "Plik nie istnieje 😞" << endl;
            return -1;
        }
    }

    BMP(int32_t szerokosc, int32_t wysokosc, bool has_alpha = true) {
        if (szerokosc <= 0 || wysokosc <= 0) {
            cout << "Wymiary nie mogą być ujemne" << endl;
        }

        naglowek_DIB.szerokosc = szerokosc;
        naglowek_DIB.wysokosc = wysokosc;
        if (has_alpha) {
            naglowek_DIB.rozmiar = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            naglowek_pliku.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);

            naglowek_DIB.bit_count = 32;
            naglowek_DIB.compression = 3;
            row_stride = szerokosc * 4;
            data.resize(row_stride * wysokosc);
            naglowek_pliku.rozmiar_pliku = naglowek_pliku.offset_data + data.size();
        }
        else {
            naglowek_DIB.rozmiar = sizeof(BMPInfoHeader);
            naglowek_pliku.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

            naglowek_DIB.bit_count = 24;
            naglowek_DIB.compression = 0;
            row_stride = szerokosc * 3;
            data.resize(row_stride * wysokosc);

            uint32_t new_stride = make_stride_aligned(4);
            naglowek_pliku.rozmiar_pliku = naglowek_pliku.offset_data + static_cast<uint32_t>(data.size()) + naglowek_DIB.wysokosc * (new_stride - row_stride);
        }
    }

    void write(const char* fname) {
        ofstream of{ fname, ios_base::binary };
        if (of) {
            if (naglowek_DIB.bit_count == 32) {
                write_headers_and_data(of);
            }
            else if (naglowek_DIB.bit_count == 24) {
                if (naglowek_DIB.szerokosc % 4 == 0) {
                    write_headers_and_data(of);
                }
                else {
                    uint32_t new_stride = make_stride_aligned(4);
                    vector<uint8_t> padding_row(new_stride - row_stride);

                    write_headers(of);

                    for (int y = 0; y < naglowek_DIB.wysokosc; ++y) {
                        of.write((const char*)(data.data() + row_stride * y), row_stride);
                        of.write((const char*)padding_row.data(), padding_row.size());
                    }
                }
            }
            else {
                cout << "Niepoprawna liczba bitow" << endl;
            }
        }
        else {
            cout << "Nie można otworzyc pliku" << endl;
        }
    }

    void histogramBarw() {
        int histogram[3][256] = { { 0 }, { 0 }, { 0 } };
        uint32_t channels = naglowek_DIB.bit_count / 8;
        for (uint32_t y = 0; y < naglowek_DIB.wysokosc; ++y) {
            for (uint32_t x = 0; x < naglowek_DIB.szerokosc; ++x) {
                histogram[0][data[channels * (y * naglowek_DIB.szerokosc + x) + 2]]++; //czerwien
                histogram[1][data[channels * (y * naglowek_DIB.szerokosc + x) + 1]]++; //zielen
                histogram[2][data[channels * (y * naglowek_DIB.szerokosc + x) + 0]]++; //niebieski
                //okresla wartosc czerwonej barwy (R) w pikselu o wspolrzednych x y

            }
        }

        tabela(histogram);
    }

    void histogramJasnosc() {
        int histogram[256] = { 0 };
        uint32_t channels = naglowek_DIB.bit_count / 8;
        for (uint32_t y = 0; y < naglowek_DIB.wysokosc; ++y) {
            for (uint32_t x = 0; x < naglowek_DIB.szerokosc; ++x) {

                int red = data[channels * (y * naglowek_DIB.szerokosc + x) + 2]; //R
                int green = data[channels * (y * naglowek_DIB.szerokosc + x) + 1]; //G
                int blue = data[channels * (y * naglowek_DIB.szerokosc + x) + 0]; //B

                if (red == green && red == blue) {
                    histogram[red]++;
                }
            }
        }
        for(int i=0; i<256; i++){
            cout<<i<<"\t"<<histogram[i]<<endl;
        }

        ofstream plik("tabelka.txt", ofstream::out);

        for(int i=0;i<256;i++){
        plik<<i<<"\t"<<histogram[i]<<endl;
        }
        plik.close();
        wyswietl_wykres_szarych(histogram);
        }

    void tabela(int histogram[3][256]) {
        int liczbaKratek = 73;

        for (int j = 0; j < liczbaKratek; j++) {
            cout << "#";
        }
        cout << endl;
        cout << "\t# R\t# G\t# B\t" << endl;

        for (int j = 0; j < liczbaKratek; j++) {
            cout << "#";
        }
        cout << endl;
        for (int i = 0; i < 256; i++) {
            cout << i << "\t";
            for (int j = 0; j < 3; j++) {
                cout << histogram[j][i] << "\t";
            }
            cout << endl;
        }

    }
    void set_pixel(uint32_t x0, uint32_t y0, uint8_t B, uint8_t G, uint8_t R, uint8_t A) {
        if (x0 > (uint32_t)naglowek_DIB.szerokosc || y0 > (uint32_t)naglowek_DIB.wysokosc) {
            throw runtime_error("Punkt znajduje sie poza granicami obrazu!");
        }

        uint32_t channels = naglowek_DIB.bit_count / 8;
        data[channels * (y0 * naglowek_DIB.szerokosc + x0) + 0] = B;
        data[channels * (y0 * naglowek_DIB.szerokosc + x0) + 1] = G;
        data[channels * (y0 * naglowek_DIB.szerokosc + x0) + 2] = R;
        if (channels == 4) {
            data[channels * (y0 * naglowek_DIB.szerokosc + x0) + 3] = A;
        }
    }

    void tabela_kolor_do_pliku(){
        fstream plik("tabelka.txt", ios::out);

        int histogram[3][256] = { { 0 }, { 0 }, { 0 } };
        uint32_t channels = naglowek_DIB.bit_count / 8;
        for (uint32_t y = 0; y < naglowek_DIB.wysokosc; ++y) {
            for (uint32_t x = 0; x < naglowek_DIB.szerokosc; ++x) {
                histogram[0][data[channels * (y * naglowek_DIB.szerokosc + x) + 2]]++; //czerwien
                histogram[1][data[channels * (y * naglowek_DIB.szerokosc + x) + 1]]++; //zielen
                histogram[2][data[channels * (y * naglowek_DIB.szerokosc + x) + 0]]++; //niebieski
                //okresla wartosc czerwonej barwy (R) w pikselu o wspolrzednych x y

            }
        }

        int liczbaKratek = 73;

        for (int j = 0; j < liczbaKratek; j++) {
            plik << "#";
        }
        plik << endl;
        plik << "\t# R\t# G\t# B\t" << endl;

        for (int j = 0; j < liczbaKratek; j++) {
            plik<< "#";
        }
        plik << endl;
        for (int i = 0; i < 256; i++) {
            plik << i << "\t";
            for (int j = 0; j < 3; j++) {
                plik << histogram[j][i] << "\t";
            }
            plik << endl;
        }


        plik.close();
        wyswietl_kolor_tabela("tabelka.txt");
        wyswietl_wykres_kolorow(histogram);
    }

    void wyswietl_kolor_tabela(string tabelka){
        fstream tabela(tabelka,ios::in);
        string linia;

        if(!tabela.good()){
            return;
        }

        while(!tabela.eof()){
            getline(tabela, linia);
            cout<<linia<<endl;
        }
        tabela.close();


    }

    void wyswietl_wykres_kolorow(int histogram[3][256]){

        for(int i=0;i<3;i++){
        for(int j=0;j<256;j++){
        int max_liczba_gwiazdek=100;
        int max_liczba_pikseli=100000;
        int liczba_pikseli=histogram[i][j];


        int liczba_gwiazdek = max_liczba_gwiazdek * liczba_pikseli / max_liczba_pikseli;

        cout<<j<<" ";

        for(int i=0;i<liczba_gwiazdek;i++){
            cout<<"x";
        }
            cout<<endl;}}

        ofstream plik("wykres.txt", ofstream::out);

        for(int i=0;i<3;i++){
        for(int j=0;j<256;j++){
        plik<<j<<"\t"<<histogram[i][j]<<endl;
        }
        cout<<endl;
        }
        plik.close();

    }

    void wyswietl_wykres_szarych(int histogram[256]){

        for(int j=0;j<256;j++){
        int max_liczba_gwiazdek=100;
        int max_liczba_pikseli=100000;
        int liczba_pikseli=histogram[j];


        int liczba_gwiazdek = max_liczba_gwiazdek * liczba_pikseli / max_liczba_pikseli;

        cout<<j<<" ";
        for(int i=0;i<liczba_gwiazdek;i++){
            cout<<"x";
        }
            cout<<endl;}

        ofstream plik("tab.txt", ofstream::out);

        for(int i=0;i<256;i++){
        plik<<i<<"\t"<<histogram[i]<<endl;
        }
        plik.close();


    }

    uint32_t row_stride{ 0 };

    void write_headers(ofstream& of) {
        of.write((const char*)& naglowek_pliku, sizeof(naglowek_pliku));
        of.write((const char*)& naglowek_DIB, sizeof(naglowek_DIB));
        if (naglowek_DIB.bit_count == 32) {
            of.write((const char*)& tablica_kolorow, sizeof(tablica_kolorow));
        }
    }

    void write_headers_and_data(ofstream& of) {
        write_headers(of);
        of.write((const char*)data.data(), data.size());
    }

    uint32_t make_stride_aligned(uint32_t align_stride) {
        uint32_t new_stride = row_stride;
        while (new_stride % align_stride != 0) {
            new_stride++;
        }
        return new_stride;
    }


    void sprawdz_color_header(BMPColorHeader& tablica_kolorow) {
        BMPColorHeader oczekiwany_color_header;
        if (oczekiwany_color_header.R_mask != tablica_kolorow.R_mask ||
            oczekiwany_color_header.B_mask != tablica_kolorow.B_mask ||
            oczekiwany_color_header.G_mask != tablica_kolorow.G_mask ||
            oczekiwany_color_header.alpha_mask != tablica_kolorow.alpha_mask) {
            cout << "Nieznany mask format" << endl;
        }
        if (oczekiwany_color_header.color_space_type != tablica_kolorow.color_space_type) {
            cout << "Program oczekuje wartosci RGB" << endl;
        }
    }


};

bool otworzObrazek(string sNazwaPliku)
{
    ifstream plik;
    plik.open(sNazwaPliku.c_str());
    if (!plik.good())
        return false;

    string wiersz;
    while (getline(plik, wiersz))
        cout << wiersz << endl;

    plik.close();
    return true;
}

int main() {
        cout << "Podaj nazwe obrazka, ktorego histogram chcesz zobaczyc:" << endl;
        char nazwaObrazka[100];
        cin>>nazwaObrazka;
        BMP obrazek;
        if(obrazek.read(nazwaObrazka)==-1){
            return 0;
        }
        system("cls");
        int option=-1;
        while(option!=0){
            cout<<"0. Wyjscie z programu"<<endl;
            cout<<"1. Histogram barw"<<endl;
            cout<<"2. Histogram jasnosci"<<endl;
            cout<<endl;
            cin>>option;
            cout<<endl;
            switch (option){
            case 0:
                cout<<"Wyjscie z programu"<<endl;
                break;
            case 1:
                obrazek.tabela_kolor_do_pliku();
                break;
            case 2:
                obrazek.histogramJasnosc();
                break;
            default:
                cout<<"Podaj poprawna opcje"<<endl;
            }
            getchar();
            getchar();
            system("cls");
    }
}
