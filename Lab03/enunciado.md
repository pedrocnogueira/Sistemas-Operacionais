LAB3 – INF1316 – Sistemas Operacionais – Paralelismo e concorrência com memória compartilhada

Ex1: Paralelismo

Criar um vetor a de 10.000 posições inicializado com valor 10. Criar 10 processos trabalhadores que utilizam áreas diferentes do vetor para multiplicar cada elemento da sua parcela do vetor por 2 armazenando esse resultado no vetor e para somar as posições do vetor retornando o resultado para um processo coordenador que irá apresentar a soma de todas as parcelas recebidas dos trabalhadores.

Obs: O 1º trabalhador irá atuar nas primeiras 1.000 posições, o 2º trabalhador nas 1.000 posições seguintes e assim sucessivamente.

Ex2: concorrência

Considere o vetor de 10.000 posições inicializado com o valor 10. Crie 10 trabalhadores, ambos multiplicam por 2 e somam 2 em todas as posições do vetor. Verifique automaticamente se todas as posições do vetor resultante têm valores iguais e explique o que ocorreu.