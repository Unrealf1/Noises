* Algorithms
  - White noise
  + Perlin noise
      + bicubic interpolation
      + make generic interpolation implementations and use one in perlin
  - Simplex noise
  - Other colored noises, i.e. brown

* Visualization
  + Render loop
    + init/uninit render
    + start frame / end frame
    + init/uninit window
    + react to window changes & stop app request
    + Init texture
    + Draw texture
  + UI via imgui
  + user input reaction systems
  + Render grid whith colored cells
  + Pan & zoom over grid
  + Keyboard control to pan & zoom
  + Zoom to mouse / center
  + Adjustable grid size
  * Proper ui
    + display basic information abour current texture and inspector state
    + Generate texture on button press
    + Generation parameters
    ? See statistics (what statisctics?)
  - Custom additional visualization per algorithm
    -   Grid vectors for perlin
    -   Places of "true" pixels in interpolation
  + Rework Pan with 2d camera in mind
  - Infinite grid
  ? Save texture to a file

  - Refactor generation interaction with ui
    -   Ui should update its values of cur tex size from on appear event, not set this by hand
    -   Sending generation time by event might be excessive (might be useful for multithread though)
    -   Delete old texture on appearence of a new one?
  + Create texture with multiple threads
  - Look into using plain array instead of memory allegro texture
  - Rework input
    Display has system events as child/sends events to the display entity
    Events are sent to the exact display

