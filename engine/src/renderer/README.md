# Renderer Lifecycle
```
                 Initialize
                     |
                     V
    ---------> Prepare Frame
    |                |
    |                V
    |        Set State on GPU
    |                |
    |                V
    |            Draw Frame
    |                |
    |                V
    |---YES--- If Still Running ---NO---|
                                        |
                                        V
                                     ShutDown
    
```