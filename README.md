# POIW-Project
Projekt na zajęcia programowanie obiektowe i wieloplatformowe.

Aplikacja webowa OCR - rozpoznawanie tekstu z obrazu.

Funkcjonalności minimalne, czyli kiedy uznajemy projekt ze zrealizowany: 
- Po uploadzie obrazu i kliknięciu przycisku pojawia się tekst, można go zaznaczyć, edytować, skopiować itd.

Funkcje, które chcielibyśmy wprowadzić, ale są opcjonalne:
- logowanie się użytkownika
- historia plików
- eksport do pdf
- obsługa wielu stron na raz
- obsługa polskich znaków
- obsługa zdjęć i pisma odręcznego.

Ogólna struktura aplikacji:
- Frontend - Angular
- Backend -  Spring Boot
- OCR - Tess4J lub inny wybrany w toku wykonywania projektu
- Baza danych - PostgreSQL

Dalej opcjonalnie:
- Spakowanie całości w kontener Docker
- Deploy za pomocą Railway

Podział obowiązków:
- Mateusz Miłowidow (Gwenrin) - Team leader, backend, OCR, baza danych, Git
- Szymon Maćkowiak (OryginalnaNazwa) - backend, OCR, baza danych
- Adam Krawczyński (Indien-ak) - Frontend, dokumentacja
- Jakub Masalski (MadmanJacob) - frontend, dokumentacja


# OBSŁUGA PROGRAMU NA WINDOWS
Na własnej maszynie należy dodać zmienną środowiskową tessdata, która domyślnie znajduje się pod:

C:\Program Files\Tesseract-OCR\tessdata

# OBSŁUGA PROGRAMU NA LINUX
Na własnej maszynie należy dodać zmienną środowiskową tessdata, która domyślnie znajduje się pod: /usr/share/tesseract-ocr/4.00/tessdata

Podana wersja jest przykładowa i należy ją zmienić odpowiednio do zainstalowanej