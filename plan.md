* Algorithms
  - White noise
  - Perlin noise
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
  - Zoom to mouse / center
  * Adjustable grid size
  * Proper ui
    + display basic information abour current texture and inspector state
    * Generate texture on button press
    - Generation parameters
    ? See statistics (what statisctics?)
  - Infinite grid
  ? Save texture to a file

  - Rework input
    Display has system events as child/sends events to the display entity
    Events are sent to the exact display

