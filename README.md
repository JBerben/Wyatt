# Wyatt API

[![Build Status](https://img.shields.io/badge/WYATT-C%20API-red.svg?style=for-the-badge&logo=data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iaXNvLTg4NTktMSI/Pg0KPCEtLSBHZW5lcmF0b3I6IEFkb2JlIElsbHVzdHJhdG9yIDE5LjAuMCwgU1ZHIEV4cG9ydCBQbHVnLUluIC4gU1ZHIFZlcnNpb246IDYuMDAgQnVpbGQgMCkgIC0tPg0KPHN2ZyB2ZXJzaW9uPSIxLjEiIGlkPSJDYXBhXzEiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIgeG1sbnM6eGxpbms9Imh0dHA6Ly93d3cudzMub3JnLzE5OTkveGxpbmsiIHg9IjBweCIgeT0iMHB4Ig0KCSB2aWV3Qm94PSIwIDAgNTEyIDUxMiIgc3R5bGU9ImVuYWJsZS1iYWNrZ3JvdW5kOm5ldyAwIDAgNTEyIDUxMjsiIHhtbDpzcGFjZT0icHJlc2VydmUiPg0KPGNpcmNsZSBzdHlsZT0iZmlsbDojNTAyMDZDOyIgY3g9IjI1NiIgY3k9IjI1NiIgcj0iMjU2Ii8+DQo8cGF0aCBzdHlsZT0iZmlsbDojM0QxOTUyOyIgZD0iTTQ5Ni4zNDQsMzQ0LjM1MkwyMjcuMzA4LDc1LjMxNmgtNTcuMzg0djExNC43NjdoMC4wOTJsLTAuMDkyLDAuMDkybDExNC43NjcsMTE0Ljc2N3Y1Ny4xMDYNCglsLTU3LjIyOS01Ny4yMjlsLTAuMTU0LDAuMTU1di0wLjEyNEgxMTIuNTQxdjExNC43NjdoMC4xMjRsLTAuMTI0LDAuMTI0bDg1LjcyMSw4NS43MjFDMjE2LjgxOCw1MDkuNzM5LDIzNi4xNDYsNTEyLDI1Niw1MTINCglDMzY2LjMzMyw1MTIsNDYwLjM2Myw0NDIuMjAyLDQ5Ni4zNDQsMzQ0LjM1MnoiLz4NCjxwb2x5Z29uIHN0eWxlPSJmaWxsOiNCMkZBMDk7IiBwb2ludHM9IjM0Mi4wNzUsMzYyLjIzMyAzNDIuMDc1LDMwNC44NSAyODQuNjkyLDMwNC44NSAyODQuNjkyLDM2Mi4yMzMgMjI3LjMwOCwzNjIuMjMzIA0KCTIyNy4zMDgsNDE5LjYxNyAzOTkuNDU5LDQxOS42MTcgMzk5LjQ1OSwzNjIuMjMzICIvPg0KPHBvbHlnb24gc3R5bGU9ImZpbGw6I0ZGREE0NDsiIHBvaW50cz0iMjI3LjMwOCwxMzIuNjk5IDIyNy4zMDgsNzUuMzE2IDE2OS45MjUsNzUuMzE2IDE2OS45MjUsMTkwLjA4MyAyMjcuMzA4LDE5MC4wODMgDQoJMjI3LjMwOCwyNDcuNDY2IDI4NC42OTIsMjQ3LjQ2NiAyODQuNjkyLDEzMi42OTkgIi8+DQo8cG9seWdvbiBzdHlsZT0iZmlsbDojRkZBNzMzOyIgcG9pbnRzPSIyODQuNjkyLDI0Ny40NjYgMjg0LjY5MiwxMzIuNjk5IDIyNy4zMDgsMTkwLjA4MyAyMjcuMzA4LDI0Ny40NjYgIi8+DQo8cG9seWdvbiBzdHlsZT0iZmlsbDojNTdBNEZGOyIgcG9pbnRzPSIzNDIuMDc1LDMwNC44NSAyODQuNjkyLDMwNC44NSAyODQuNjkyLDI0Ny40NjYgMzk5LjQ1OSwyNDcuNDY2IDM5OS40NTksMzYyLjIzMyANCgkzNDIuMDc1LDM2Mi4yMzMgIi8+DQo8cG9seWdvbiBzdHlsZT0iZmlsbDojMjQ4OEZGOyIgcG9pbnRzPSIzNDIuMDc1LDM2Mi4yMzMgMzk5LjQ1OSwzNjIuMjMzIDM5OS40NTksMjQ3LjQ2NiAzNDIuMDc1LDMwNC44NSAiLz4NCjxyZWN0IHg9IjExMi41NCIgeT0iMzA0Ljg1MSIgc3R5bGU9ImZpbGw6I0Y3QUZEMTsiIHdpZHRoPSIxMTQuNzY2IiBoZWlnaHQ9IjExNC43NjYiLz4NCjxwb2x5Z29uIHN0eWxlPSJmaWxsOiNGMDcyQUU7IiBwb2ludHM9IjExMi41NDEsNDE5LjYxNyAyMjcuMzA4LDQxOS42MTcgMjI3LjMwOCwzMDQuODUgIi8+DQo8cG9seWdvbiBzdHlsZT0iZmlsbDojOUNERDA1OyIgcG9pbnRzPSIzNDIuMDc1LDM2Mi4yMzMgMzQyLjA3NSwzMDUuMDk3IDIyNy41NTUsNDE5LjYxNyAzOTkuNDU5LDQxOS42MTcgMzk5LjQ1OSwzNjIuMjMzICIvPg0KPGc+DQo8L2c+DQo8Zz4NCjwvZz4NCjxnPg0KPC9nPg0KPGc+DQo8L2c+DQo8Zz4NCjwvZz4NCjxnPg0KPC9nPg0KPGc+DQo8L2c+DQo8Zz4NCjwvZz4NCjxnPg0KPC9nPg0KPGc+DQo8L2c+DQo8Zz4NCjwvZz4NCjxnPg0KPC9nPg0KPGc+DQo8L2c+DQo8Zz4NCjwvZz4NCjxnPg0KPC9nPg0KPC9zdmc+DQo=)](https://travis-ci.org/joemccann/dillinger)

#### Wyatt is an AI based bot, that competes against other players in Tetris99, for the Nintendo Switch.

  - This API is for a Python extension, to communicate with the Console.
  - Communication Protocol is mainly Bluetooth focused.
  - Possible Serial support in the future.

### Todos

 - Add more to this Readme.
 - Finish the project...
