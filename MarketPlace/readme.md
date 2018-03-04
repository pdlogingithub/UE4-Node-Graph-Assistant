# UE4-Node-Graph-Assistant User Guide

Right click on pins to connect multiple nodes while dragging a connection wire.  
![1](right_click_multi_drop.gif)  

Click on pin to start free panning,zooming and dropping.  
(hold down shift will also enter this mode)  
![7](003_click_multi_drop.gif)  

Shift click on connected node pin to duplicate connection wire.  
![4](duplicate.gif)  

Double click on pin to highlight all connected,single click to highlight single wire,shift toggle highlight.    
 (for function nodes,slowly double click to avoid openning the node)  
![6](007_cluster_highlight.gif)  

 Double click on node to selecte all connected nodes.  
 depend on left or right area of the node clicked,will select all children or all parents.  
 (for function nodes,move mouse a little bit between two click to avoid openning the node)  
![7](008_stream.gif)

Left click and drag on empty space while holding down alt will break all connection wires along its way.  
(note that this is a experimental feature,if you are dragging too fast,some connection wires may still stay connected).  
![8](break.gif)


QnA:  
Does it support blueprint or other type of node graph?   
Yes,it supports blueprint,also generally every kind of node graphs like animation blueprint or behavior tree graph.I mostly test on material graph and blueprint graph.  

Will it support other platform and engine version?  
I'll try to support more platform and engine version in the future if there is no technical difficulty.  

Tips:  
 
