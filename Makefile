scripts = usefulScripts
dataFolder = comiFolder
files = ${dataFolder}/files
data = ${dataFolder}/comiData

build:
	./${scripts}/makeFolder.sh
	./${scripts}/systemInit.sh ${data} ${files}

all: build install run

install:
	./${scripts}/installFS.sh

run:
	./${scripts}/mountFolder.sh 

clean:
	rm -rf *Folder
	rm comiFS*

restore:
	./${scripts}/restoreFiles.sh dataFolder