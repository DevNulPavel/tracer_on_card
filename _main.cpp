// https://tproger.ru/translations/business-card-raytracer/

#include <stdlib.h>   // card > aek.ppm
#include <stdio.h>
#include <math.h>

#include <png.h>

#include <omp.h>
#if _OPENMP < 200805
#define collapse(x)
#endif
#define CHUNK 50

#include "Types.h"
#include "ToPng.h"



typedef int i;       // Экономим место с помощью сокращения int до i
typedef float f;     // Экономим еще больше места с f вместо float

// Класс вектора с конструктором и операторами
struct vector {
    float x,y,z;  // Три координаты вектора

    ///////
    // Конструктор
    vector(float a,float b,float c) {
        x=a;
        y=b;
        z=c;
    }
    // Пустой конструктор
    vector() {

    }
    // Сумма векторов
    vector operator+(vector r) const {
        return vector(x+r.x,y+r.y,z+r.z);
    }
    // Масштабирование векторов
    vector operator*(float r)  const {
        return vector(x*r,y*r,z*r);
    }
    // Скалярное произведение векторов
    float operator%(vector r) const {
        return x*r.x+y*r.y+z*r.z;
    }
    // Векторное произведение векторов
    vector operator^(vector r) const {
        return vector(y*r.z-z*r.y,z*r.x-x*r.z,x*r.y-y*r.x);
    }
    // Нормализация вектора
    vector operator!() const {
        return *this*(1 /sqrt(*this%*this));
    }
};

// Набор позиций сфер, описывающий мир
// Все эти числа, по сути, являются по сути битовым вектором
int G[]= {247570,280596,280600,249748,18578,18577,231184,16,16};

// Генератор случайных чисел, возвращающий число с плавающей точкой в диапазоне 0-1
float R() {
    return(f)rand()/RAND_MAX;
}

// Тест на пересечение для линии [o,v]
// Возвращаем 2, если была задета сфера (а также дистанцию пересечения t и полу-вектор n).
// Возвращаем 0, если луч ничего не задевает и идет вверх, в небо
// Возвращаем 1, если луч ничего не задевает и идет вниз, в пол
int T(const vector& o, const vector& d, f& t, vector& n) {
    t = 1e9;
    int m = 0;
    float p = -o.z/d.z;
    if(0.01<p) {
        t=p;
        n=vector(0,0,1);
        m=1;
    }

    // Мир зашифрован в G, в 9 линий и 19 столбцов
    for(int k=19; k>=0; k--) { // Для каждого столбца
        for(int j=9; j>=0; j--) { // Для каждой строки

            if(G[j]&1 << k) { // Для этой линии j есть ли в столбце int cфера?
                // Сфера есть, но задевает ли ее луч?
                vector p = o + vector(-k,0,-j-4);
                float b = p%d;
                float c = p%p-1;
                float q = b*b-c;

                // Задевает ли луч сферу?
                if(q > 0) {
                    // Да. Считаем расстояние от камеры до сферы
                    float s=-b-sqrt(q);

                    if(s<t && s>0.01) {
                        // Это минимальное расстояние, сохраняем его. А также
                        // вычиваем вектор отскакивающего луча и записываем его в 'n'
                        t=s,
                        n=!(p+d*t),
                        m=2;
                    }
                }
            }
        }
    }

    return m;
}

// (S)эмплируем мир и возвращаем цвет пикселя по
// по лучу, начинающемуся в точке o (Origin) и имеющему направление d (Direction)
vector S(const vector& o, const vector& d) {
    float t;
    vector n;

    // Проверяем, натыкается ли луч на что-нибудь
    int m = T(o,d,t,n);

    if(!m) { // m==0
        // Сфера не была найдена, и луч идет вверх: генерируем цвет неба
        return vector(0.7, 0.6, 1) * pow(1-d.z, 4);
    }

    // Возможно, луч задевает сферу
    vector h = o+d*t;                    // h - координата пересечения
    vector l = !(vector(9+R(),9+R(),16)+h*-1);  // 'l' = направление света (с небольшим искажеем для эффекта мягких теней)
    vector r = d+n*(n%d*-2);               // r = полувектор

    // Расчитываем коэффицент Ламберта
    float b = l%n;

    // Рассчитываем фактор освещения (коэффицент Ламберта > 0 или находимся в тени)?
    if(b<0 || T(h,l,t,n)) {
        b=0;
    }

    // Рассчитываем цвет p (с учетом диффузии и отражения света)
    float p=pow(l%r*(b>0),99);

    if(m&1) {  // m == 1
        h=h*0.2; // Сфера не была задета, и луч уходит вниз, в пол: генерируем цвет пола
        return((i)(ceil(h.x)+ceil(h.y))&1?vector(3,1,1):vector(3,3,3))*(b*.2+.1);
    }

    // m == 2 Была задета сфера: генерируем луч, отскакивающий от поверхности сфера
    return vector(p,p,p)+S(h,r)*0.5; // Ослабляем цвет на 50%, так как он отскакивает от поверхности (* .5)
}


// Главная функция. Выводит изображение.
// Использовать программу просто: ./card > erk.ppm
int main() {
    // Оператор "!" осуществляет нормализацию вектора
    vector g = !vector(-12,-26,0);       // Направление камеры
    vector a = !(vector(0,0,1)^g) * 0.002; // Вектор, отвечающий за высоту камеры...
    vector b = !(g^a) * 0.002;          // Правый вектор, получаемый с помощью векторного произведения
    vector c = (a+b)*(-256) + g;       // WTF? Вот здесь https:// news.ycombinator.com/item?id=6425965 написано про это подробнее.

    const size_t size = 512;
    const size_t bufferSize = size * size;
    Color* buffer = (Color*)malloc(bufferSize * sizeof(Color));

    // omp_set_num_threads(4);
    #pragma omp parallel for schedule(dynamic, CHUNK) collapse(2)
    for(int y = 0; y < size; y++) {  // Для каждого столбца
        for(int x = 0; x < size; x++) { // Для каждого пикселя в строке
            // Используем класс вектора, чтобы хранить цвет в RGB
            vector p(13,13,13);     // Стандартный цвет пикселя — почти черный

            // Бросаем по 64 луча из каждого пикселя
            for(int r=64; r--; ) {
                // Немного меняем влево/вправо и вверх/вниз координаты начала луча (для эффекта глубины резкости)
                vector t = a*(R()-0.5)*99 + b*(R()-0.5)*99;

                // Назначаем фокальной точкой камеры vector(17,16,8) и бросаем луч
                // Аккумулируем цвет, возвращенный в переменной t
                // Начало луча
                vector start = vector(17,16,8)+t;
                // Направление луча с небольшим искажением
                // ради эффекта стохастического сэмплирования
                vector direction = !(t*(-1) + (a*(R()+x) + b*(y+R()) + c)*16);
                p = S(start, direction)*3.5 + p; // +p для аккумуляции цвета
            }

            //#pragma omp ordered
            //printf("%c%c%c", (i)p.x, (i)p.y, (i)p.z);
            size_t index = y*size+x;
            buffer[index].r = (char)p.x;
            buffer[index].g = (char)p.y;
            buffer[index].b = (char)p.z;
        }
    }

    // Заголовок PPM
    // printf("P6 512 512 255 ");
    // for (size_t i = bufferSize-1; i >=0; --i){
    //     printf("%c%c%c", buffer[i].r, buffer[i].g, buffer[i].b);
    // }

    int result = writeImage("out.png", size, size, buffer);

    free(buffer);
}