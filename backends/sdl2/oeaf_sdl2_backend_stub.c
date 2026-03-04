/*
  SDL2 backend placeholder (not compiled by default).

  Idea:
  - attach: SDL_Init(...)
  - OS.pump: SDL_PollEvent -> translate into oeaf_event -> oeaf_events_push
  - TIMER: SDL_GetPerformanceCounter/Frequency
  - VIDEO: SDL_CreateWindow, SDL_GL_CreateContext, SDL_GL_SwapWindow...
  - INPUT: optionally maintain key/mouse state for queries

  This file is intentionally a stub for OEAF v0.1.
*/
