# UE4-Node-Graph-Assistant User Guid

Right click to connect multiple nodes while dragging a connection wire.  
![1](right_click_multi_drop.gif)  

Right click and drag while dragging a connection wire will pan the node graph.  
![2](drag_and_pan.gif)  

Hold down shift while dragging a connection allow you to freely navigate through mouse.  
![3](shift_multi_drop.gif)  

Shift click on connected node pin will duplicate connection wire.  
![4](duplicate.gif)  

 Left click on connection wire will highlight it,click on empty space to remove highlight.  
![5](highlight.gif)

Left click and drag on empty space while holding down alt will break all connection wires along its way.  
![6](break.gif)


QnA:  
Does it support blueprint or other type of node graph?   
Yes,it support blueprint,also generally every kind of node graphs like animation blueprint or behavior tree graph.I mostly test on material graph and blueprint graph.  

Will it support other platform and engine version?  
I'll try to support more platform and engine version in the future if there is no technical difficulty.  

Tips:  
It's recommended that you go to editor preference window and set "Spling Hover Tolerance" to higher than 10 for better control.  
