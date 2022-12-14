#include <stdio.h>
#include <stdlib.h>

double* addDataSet(double *ppDataSet,int *dataSetSize, double data);

int main(){

    double *dataSet = NULL;
    double **pDS = &dataSet;
    int dataSetSize = 0;

    stamFunc(pDS,&dataSetSize);

    printf("___________________________\n");
    printData(dataSet,dataSetSize);
    printf("___________________________\n");
    printf("%f\n",dataSet[0]);
    printf("%f\n",dataSet[1]);

}

void stamFunc(double **dataSet,int *dataSetSize){
    *dataSet = addDataSet(*dataSet,dataSetSize,1.0);
    *dataSet = addDataSet(*dataSet,dataSetSize,2.0);
    *dataSet = addDataSet(*dataSet,dataSetSize,3.0);
}


double* addDataSet(double *dataSet,int *dataSetSize, double data)
{
    printf("\nAdding data to data set\n");
    printData(dataSet,*dataSetSize);
    double *newDataSet = realloc(dataSet, (*dataSetSize + 1) * sizeof(double));
    if (newDataSet == NULL)
    {
        printf("Error reallocating memory\n");
        return NULL;
    }
    newDataSet[*dataSetSize] = data;
    (*dataSetSize)++;
    printf("Data added\n");
    printData(newDataSet,*dataSetSize);
    return newDataSet;
}

void printData(double *dataSet,int size){
    printf("    Printing data set. Size : %d\n",size);
    for(int i = 0; i < size; i++){
        printf("        %0.1f\n",dataSet[i]);
    }
}
