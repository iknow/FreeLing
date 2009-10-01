#include "freeling/similarity.h"
#include "freeling/traces.h"

#define MOD_TRACENAME "SIMILARITY"
#define MOD_TRACECODE SIMILARITY_TRACE

using namespace std;

///////////////////////////////////////////////////////////////
///  Creator
///////////////////////////////////////////////////////////////

similarity::similarity() {
	

}


///////////////////////////////////////////////////////////////
///  Calculate the distance between two words 
///////////////////////////////////////////////////////////////

similarity::~similarity(){

	
 
}

int similarity::ComputeDistance(string s, string t)
{
    int n=s.size();
    int m=t.size();
    int distance[50][50]; // matrix

    int cost=0;
    if(n == 0) return m;
    if(m == 0) return n;
    //init1

    int i=0;
    int j=0;
    for(i=0; i <= n; i++) distance[i][0]=i;
    for(j=0; j <= m; j++) distance[0][j]=j;
    //find min distance

    for(i=1; i <= n; i++){
        for(j=1; j <= m;j++){
            	cost=(t.substr(j - 1, 1) == 
                s.substr(i - 1, 1) ? 0 : 1);
           // distance[i][j]=Min3(distance[i - 1][j] + 1,distance[i][j - 1] + 1,distance[i - 1][j - 1] + cost);
		int valor[3];
		valor[0]=distance[i - 1][j] + 1;
		valor[1]=distance[i][j - 1] + 1;
		valor[2]=distance[i - 1][j - 1] + cost;
		int minimo=valor[0];
        	for (int indice=1;indice<3;indice++)
        	{	
			if (minimo>valor[indice]) {minimo=valor[indice];}
		}
        	distance[i][j]=minimo;
	}
    }
    return distance[n][m];
}

///////////////////////////////////////////////////////////////
///  Returns the similarity between two words 
///////////////////////////////////////////////////////////////

float similarity::getSimilarity(string word1, string word2){

     	
    float dis=ComputeDistance(word1, word2);
    float maxLen=word1.size();
    if (maxLen < word2.size())
    maxLen = word2.size();
    if (maxLen == 0.0F)
    return 1.0F;
    else
    return 1.0F - dis/maxLen; 
}

