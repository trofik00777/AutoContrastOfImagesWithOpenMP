#include <iostream>
#include <stdio.h>
#include <string>

#include <fstream>

#include <omp.h>

using namespace std;

struct pix {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

struct brightness {
    int min;
    int max;
};

unsigned char convert(float x, float max_br, float min_br) {
    if (max_br <= min_br) {
        return max_br;
    }
    int answ = ((x - min_br) / (max_br - min_br)) * 255;
    if (answ > 255) {
        return 255;
    }
    if (answ < 0) {
        return 0;
    }
    return answ;
}

int max(int a, int b) {
    if (a >= b) {
        return a;
    }
    return b;
}

int min(int a, int b) {
    if (a <= b) {
        return a;
    }
    return b;
}

void arrprint(int *arr) {
    for (int i = 0; i < 255; i++) {
        printf("<%d:%d> ", i, arr[i]);
    }
    printf("\n");
}

brightness find_min_max(int *pixels_counter, int propusk) {
    int propusk_min = propusk, propusk_max = propusk;
    int min_br, max_br;

    bool found_min = false, found_max = false;

//    arrprint(pixels_counter);

    for (int i = 0; i < 256; i++) {
        if (!found_min && pixels_counter[i] > 0) {
            if (pixels_counter[i] > propusk_min) {
                min_br = i;
                found_min = true;
                if (found_max) {
                    break;
                }
            } else {
                propusk_min -= pixels_counter[i];
            }
        }
        if (!found_max && pixels_counter[255 - i] > 0) {
            if (pixels_counter[255 - i] > propusk_max) {
                max_br = 255 - i;
                found_max = true;
                if (found_min) {
                    break;
                }
            } else {
                propusk_max -= pixels_counter[255 - i];
            }
        }
    }

    brightness answ{min_br, max_br};
    return answ;
}

int main(int argc, char *argv[])
{
    if (argc < 5) {
        printf("Not 4 args...");
        return 0;
    }
    string name_in;
    string name_out;
    float coef;
    try {
        name_in = argv[2];
        name_out = argv[3];
        coef = stof(argv[4]);

    } catch (const std::exception&error) {
        printf("Error!\n");
        return 0;
    }
    if (coef >= 0.5 || coef < 0.0) {
        printf("Incorrect coeff %f\n", coef);
        return 0;
    }

    std::ifstream infile(name_in, fstream::in | fstream::binary);

    if (!infile.is_open()) {
        printf("File not found...");
        return 0;
    }
    int num_thr1;
    try {
        num_thr1 = stoi(argv[1]);
    } catch (const std::exception&error) {
        printf("Incorrect number of thread(s)");
        return 0;
    }
    if (num_thr1 < 1) {
        num_thr1 = omp_get_max_threads();
    }
    const int num_thr = num_thr1;
    omp_set_num_threads(num_thr);
    char c;
    infile.get(c);

    if (c == 'P') {
        infile.get(c);
        char mode_ppm = c;

        if (mode_ppm != '5' && mode_ppm != '6') {
            printf("Sorry, but this format doesn't supported");
            infile.close();

            return 0;
        }

        infile.get(c);
        infile.get(c);

        char wi[20], hi[20], maxi[20];
        int pos_wi = 0, pos_hi = 0, pos_maxi = 0;
        int w = 0;
        while (c != ' ' && c != '\n') {
            wi[pos_wi++] = c;
            w = (w * 10) + (c - '0');
            infile.get(c);
        }
        int h = 0;
        infile.get(c);
        while (c != ' ' && c != '\n') {
            hi[pos_hi++] = c;
            h = (h * 10) + (c - '0');
            infile.get(c);
        }
        int maximum = 0;
        infile.get(c);
        while (c != ' ' && c != '\n') {
            maxi[pos_maxi++] = c;
            maximum = (maximum * 10) + (c - '0');
            infile.get(c);
        }

        if (mode_ppm == '6') {
//            printf("width: %d\nheight: %d\nmax_color(?): %d\n", w, h, maximum);

            pix *pixels = new pix[h * w];
            int max_br = 0, min_br = 255;
            int propusk = ((float) (h * w)) * coef;
            int pixels_counter_r[256]{};
            int pixels_counter_g[256]{};
            int pixels_counter_b[256]{};

            for (int i = 0; i < h * w; i++) {
                unsigned char c1;

                infile.get(c);
                c1 = (unsigned char) c;
                pixels[i].r = c1;

                infile.get(c);
                c1 = (unsigned char) c;
                pixels[i].g = c1;

                infile.get(c);
                c1 = (unsigned char) c;
                pixels[i].b = c1;
            }
            infile.close();

            double start_time = omp_get_wtime();

#pragma omp parallel default(none) shared(h, w, pixels, pixels_counter_r, pixels_counter_g, pixels_counter_b)
            {
                int buffer_r[256] = {0};
                int buffer_g[256] = {0};
                int buffer_b[256] = {0};
#pragma omp for schedule(static)
                for (int i = 0; i < h * w; i++) {
                    buffer_r[pixels[i].r]++;
                    buffer_g[pixels[i].g]++;
                    buffer_b[pixels[i].b]++;
                }
#pragma omp critical (merge)
                {
                    for (int j = 0; j < 256; j++) {
                        pixels_counter_r[j] += buffer_r[j];
                        pixels_counter_g[j] += buffer_g[j];
                        pixels_counter_b[j] += buffer_b[j];
                    }
                }

            }

            brightness minmax_r;
            brightness minmax_g;
            brightness minmax_b;
#pragma omp parallel sections default(none) shared(minmax_r, minmax_g, minmax_b, pixels_counter_r, pixels_counter_g, pixels_counter_b, propusk)
            {
#pragma omp section
                minmax_r = find_min_max(pixels_counter_r, propusk);
#pragma omp section
                minmax_g = find_min_max(pixels_counter_g, propusk);
#pragma omp section
                minmax_b = find_min_max(pixels_counter_b, propusk);
            }
            min_br = minmax_r.min;
            min_br = min(min_br, min(minmax_g.min, minmax_b.min));
            max_br = minmax_r.max;
            max_br = max(max_br, max(minmax_g.max, minmax_b.max));

//            printf("min = %d\tmax = %d\n", min_br, max_br);
            if (min_br != 0 || max_br != 255) {
                unsigned char predict[256];
                for (int i = 0; i < 256; i++) {
                    predict[i] = convert(i, max_br, min_br);
                }

#pragma omp parallel default(none) shared(pixels, predict, h, w)
                {
#pragma omp for schedule(static)
                    for (int i = 0; i < h * w; i++) {
                        pixels[i].r = predict[pixels[i].r];
                        pixels[i].g = predict[pixels[i].g];
                        pixels[i].b = predict[pixels[i].b];
                    }
                }
            }

            double end_time = omp_get_wtime();
            double duration = end_time - start_time;

            printf("Time (%i thread(s)): %g ms\n", num_thr, duration * 1000);

            ofstream outfile(name_out, ios::out | ios::binary);
            outfile.put('P');
            outfile.put('6');
            outfile.put('\n');
            for (int i = 0; i < pos_wi; i++) {
                outfile.put(wi[i]);
            }
            outfile.put(' ');
            for (int i = 0; i < pos_hi; i++) {
                outfile.put(hi[i]);
            }
            outfile.put('\n');
            for (int i = 0; i < pos_maxi; i++) {
                outfile.put(maxi[i]);
            }
            outfile.put('\n');

            for (int i = 0; i < w * h; i++) {
                outfile.put(pixels[i].r);
                outfile.put(pixels[i].g);
                outfile.put(pixels[i].b);
            }

            outfile.close();

            delete[] pixels;
        } else if (mode_ppm == '5') {
//            printf("width: %d\nheight: %d\nmax_color(?): %d\n", w, h, maximum);

            unsigned char *pixels = new unsigned char[h * w];
            int max_br = 0, min_br = 255;
            int propusk = ((float) (h * w)) * coef;
//            int *pixels_counter = new int[256]{};

            int pixels_counter[256]{};

            for (int i = 0; i < h * w; i++) {
                unsigned char c1;

                infile.get(c);
                c1 = (unsigned char) c;
                pixels[i] = c1;
            }
            infile.close();

            double start_time = omp_get_wtime();

#pragma omp parallel default(none) shared(h, w, pixels, pixels_counter)
            {
                int buffer[256] = {0};
#pragma omp for schedule(static)
                for (int i = 0; i < h * w; i++) {
                    buffer[pixels[i]]++;
                }
#pragma omp critical (merge)
                {
                    for (int j = 0; j < 256; j++) {
                        pixels_counter[j] += buffer[j];
                    }
                }

            }

            brightness minmax = find_min_max(pixels_counter, propusk);
            min_br = minmax.min;
            max_br = minmax.max;

            if (min_br != 0 || max_br != 255) {
                unsigned char predict[256];
                for (int i = 0; i < 256; i++) {
                    predict[i] = convert(i, max_br, min_br);
                }

#pragma omp parallel for default(none) schedule(static) shared(h, w, pixels, predict)
                for (int i = 0; i < h * w; i++) {
                    pixels[i] = predict[pixels[i]];
                }

            }

            double end_time = omp_get_wtime();
            double duration = end_time - start_time;

            printf("Time (%i thread(s)): %g ms\n", num_thr, duration * 1000);

            ofstream outfile(name_out, ios::out | ios::binary);
            outfile.put('P');
            outfile.put('5');
            outfile.put('\n');
            for (int i = 0; i < pos_wi; i++) {
                outfile.put(wi[i]);
            }
            outfile.put(' ');
            for (int i = 0; i < pos_hi; i++) {
                outfile.put(hi[i]);
            }
            outfile.put('\n');
            for (int i = 0; i < pos_maxi; i++) {
                outfile.put(maxi[i]);
            }
            outfile.put('\n');

            for (int i = 0; i < w * h; i++) {
                outfile.put((unsigned char) pixels[i]);
            }

            outfile.close();

            delete[] pixels;
        }
    } else {
        infile.close();
        printf("File isn't P5 or P6...");
    }

    return 0;
}
