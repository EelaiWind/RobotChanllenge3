#include<cstdio>
#include<cstdlib>

int main() {
	char c;
	while(scanf("%c", &c)!=EOF) {
		if(c=='c') {
			system("gphoto2 --capture-image-and-download --filename");
		}
		else if (c=='q') break;
	}
	
	
}