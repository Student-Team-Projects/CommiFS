rm -rf comiFolder
rm -rf mountComiFolder

mkdir comiFolder
mkdir mountComiFolder
cd comiFolder
mkdir comiData
mkdir files
cd files
mkdir A
mkdir B
mkdir A/AB

echo "zawartosc pliku a.txt" > a.txt

echo "zawartosc pliku a.txt w A" > A/a.txt
echo "zawartosc pliku b.txt w B" > B/b.txt
echo "zawartosc pliku ab.txt w A/AB" > A/AB/ab.txt