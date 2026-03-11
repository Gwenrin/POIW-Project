# POIW-Project
Projekt na zajęcia programowanie obiektowe i wieloplatformowe.

Aplikacja webowa OCR - rozpoznawanie tekstu z obrazu.
Funkcjonalności minimalne, czyli kiedy uznajemy projekt ze zrealizowany: 
Po uploadzie obrazu i kliknięciu przycisku pojawia się tekst, można go zaznaczyć, edytować, skopiować itd.

Funkcje, które chcielibyśmy wprowadzić, ale są opcjonalne:
- logowanie się użytkownika
- historia plików
- eksport do pdf
- obsługa wielu stron na raz
- obsługa polskich znaków
- obsługa zdjęć i pisma odręcznego.

Ogólna struktura apki:
- Frontend - Thymeleaf albo jak się zgodzi to Angular
- Backend -  Spring Boot
- OCR - Tess4J albo inny, który będzie fajnie działać w javie lub w pythonie jeśli będzie można
- Baza danych - PostgreSQL bo MySQL trochę cringe i wiocha
Dalej opcjonalnie:
- Spakowanie całości w kontener Docker
- Deploy na Railway i działa to jako normalna strona internetowa

Podział obowiązków:
- Mateusz Miłowidow (Gwenrin) - Team leader, backend, OCR, baza danych, Git
- Szymon Maćkowiak (OryginalnaNazwa) - backend, OCR, baza danych
- Adam Krawczyński (Indien-ak) - Frontend, dokumentacja
- Jakub Masalski (MadmanJacob) - frontend, dokumentacja
