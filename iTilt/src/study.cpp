// В прошивку не входит
//
// Изучаю вопрос фильтрации готового набора данных
// Ищу варианты лучше, чем среднее арифметического
// Если построить гистограмму распределения этих данных, то 90.29 выглядит более правильным знчением среднего

#include <stdio.h>
#include <stdlib.h>

// Функция для сравнения (необходима для qsort)
int compare(const void *a, const void *b) {
    return (*(float*)a - *(float*)b);
}

// Функция для вычисления медианы
//Медиана — это значение, которое делит данные на две равные части. Она устойчива к выбросам и может быть лучше среднего арифметического, если в данных есть аномальные значения.
float median(float data[], int data_size) {
    qsort(data, data_size, sizeof(float), compare);
    if (data_size % 2 == 0) {
        return (data[data_size / 2 - 1] + data[data_size / 2]) / 2.0;
    } else {
        return data[data_size / 2];
    }
}


// Функция для вычисления усечённого среднего
// Усечённое среднее — это среднее арифметическое, вычисленное после удаления определённого процента наименьших и наибольших значений. Это помогает уменьшить влияние выбросов.
float trimmed_mean(float data[], int data_size, float trim_percent) {
    qsort(data, data_size, sizeof(float), compare);
    int trim_count = (int)(data_size * trim_percent / 100);
    float sum = 0;
    int count = 0;

    for (int i = trim_count; i < data_size - trim_count; i++) {
        sum += data[i];
        count++;
    }

    return sum / count;
}


// Функция для вычисления моды
// Мода — это значение, которое встречается в данных чаще всего. Если шум имеет случайный характер, а реальное значение повторяется чаще всего, то мода может быть хорошим выбором.
float mode(float data[], int data_size) {
    qsort(data, data_size, sizeof(float), compare);

    float current_value = data[0];
    int current_count = 1;
    float mode_value = current_value;
    int max_count = current_count;

    for (int i = 1; i < data_size; i++) {
        if (data[i] == current_value) {
            current_count++;
        } else {
            if (current_count > max_count) {
                max_count = current_count;
                mode_value = current_value;
            }
            current_value = data[i];
            current_count = 1;
        }
    }

    // Проверяем последний элемент
    if (current_count > max_count) {
        mode_value = current_value;
    }

    return mode_value;
}


float avg(float data[], int data_size) {
    float avg=0;
    for (int i = 0; i < data_size; i++) {   
        avg += data[i];
    }
    avg = avg / data_size;
    return avg;
}


int main() {
    float data[] = {90.37, 90.35, 90.26, 90.27, 90.23, 90.35, 90.31, 90.29, 90.23, 90.32, 90.24, 90.27, 90.26, 90.25, 90.32, 90.29, 90.32, 90.37, 90.29, 90.3, 90.23, 90.29, 90.28, 90.25, 90.35, 90.29, 90.34, 90.24, 90.26, 90.26, 90.3, 90.29, 90.33, 90.27, 90.3, 90.29, 90.22, 90.33, 90.36, 90.32, 90.26, 90.24, 90.31, 90.27, 90.35, 90.33, 90.35, 90.31, 90.28, 90.26, 90.26, 90.3, 90.33, 90.26, 90.29, 90.35, 90.28, 90.29, 90.26, 90.27, 90.22, 90.26, 90.28, 90.39, 90.24, 90.41, 90.32, 90.33, 90.25, 90.34, 90.39, 90.29, 90.28, 90.3, 90.33, 90.24, 90.26, 90.29, 90.31, 90.28, 90.37, 90.28, 90.27, 90.25, 90.27, 90.28, 90.29, 90.27, 90.3, 90.31, 90.24, 90.28, 90.3, 90.24, 90.34, 90.23, 90.32, 90.34, 90.31, 90.26};
    int data_size = sizeof(data) / sizeof(data[0]);

    float result = avg(data, data_size);
    printf("Среднее арифметическое: %.2f\n", result); //90.29

    result = median(data, data_size);
    printf("Медиана: %.2f\n", result); //90.26
    
    float trim_percent = 10.0;  // Удаляем 10% наименьших и наибольших значений
    result = trimmed_mean(data, data_size, trim_percent);
    printf("Усечённое среднее: %.2f\n", result); //90.29

    result = mode(data, data_size); //90.26
    printf("Мода: %.2f\n", result);    
    


    return 0;
}