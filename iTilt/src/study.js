// В прошивку не входит
//
//вспомогательные функции, которые я использовал в анализе данных акселерометра

//плотность
function calculateDensity(data) {
    let n = data.length;
    let h = 1.06 * std(data) * Math.pow(n, -0.2); /*Правило Сильвермана для выбора h*/
    let density = new Array(n).fill(0);
    
    for (let i = 0; i < n; i++) {
      for (let j = 0; j < n; j++) {
        let u = (data[i] - data[j]) / h;
        density[i] += Math.exp(-0.5 * u * u) / (Math.sqrt(2 * Math.PI)); /*Гауссово ядро*/
      }
      density[i] /= (n * h); /*Нормализация*/
    }
    return density;
  }
  
  /* Функция для расчета среднего значения */
  function mean(data) {
    return data.reduce((sum, x) => sum + x, 0) / data.length;
  }
  
  /* Функция для расчета стандартного отклонения */
  function std(data) {
    let mu = mean(data);
    let squaredDiffs = data.map(x => Math.pow(x - mu, 2));
    let variance = squaredDiffs.reduce((sum, x) => sum + x, 0) / data.length;
    return Math.sqrt(variance);
  }