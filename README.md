# UE4-Node-Graph-Assistant User Guide

support page: https://forums.unrealengine.com/unreal-engine/marketplace/1435240-node-graph-assistant  
Marketplace page: https://www.unrealengine.com/marketplace/node-graph-assistant  
中文说明：https://github.com/pdlogingithub/UE4-Node-Graph-Assistant/blob/master/Doc/%E8%99%9A%E5%B9%BB4%E8%93%9D%E5%9B%BE%E5%8A%A9%E6%89%8B%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E.md  

1.Right click multi-connect: Panning is unlock when dragging a wire,right click on pin to multi-connect,right click on panel to cancel.  
![1](Resource/1.4/drag_pan_multi-connect.gif)  

2.Left click multi-connect: Click on pin to start free panning,zooming and multi-connecting.  
![2](Resource/1.4/click_pan_multi-connect.gif)  

3.Shift click multi-connect: When dragging a wire, hold down shift to start free panning,zooming and multi-connecting.  
![3](Resource/1.4/shift_pan_multi-connect.gif)  

4.Dupli-wire: Shift click or drag on pin to duplicate wire.  
![4](Resource/1.4/dupli_wire.gif)  

5.Rearrange nodes: Press alt + r to rearrange nodes,most suitable for small block of nodes.   
![5](Resource/1.4/rearrange.gif)  

6.Bypass node: Press alt + x will remove selected nodes on wire.  
![6](Resource/1.4/bypass.gif)  

7.Create node on wire: Right click on wire to create node on wire.  
![7](Resource/1.4/insert.gif)

8.Highlight wire: Left click to highlight wire, hold down shift to highlight multiple wires,middle mouse double click on pin to highlight all connected wires.  
![8](Resource/1.4/highlight.gif)  

9.Cutoff wires: Hold down alt and drag on empty space will break all wires along its way.  
![9](Resource/1.4/cutoff.gif)  

10.Select linked node: Middle mouse double click on node to selecte all connected nodes.  
Depending on left or right area of the node clicked on,will select all children or all parents.  
Or use hot key alt+a, or alt +d.  
![10](Resource/1.4/select_linked.gif)  

11.Lazy connect: Wire will auto align to closest connectible pin of the current hovered node.  
when click and drag while hold down shit will connect all node pins under cursor.  
![11](Resource/1.5/lazy_connect.gif)  
 
12.Auto connec: When moving a node,its pins will align to surrounding connectible pins, releasing mouse will commit connections.  
hold down alt will suppress auto connect.   
![12](Resource/1.5/auto_connect.gif)  

13.Wire style: press tool bar button to cycle through wire style.  
![13](Resource/1.5/wire_style.gif)  


—————————————————1.6 pending features———————————————————  


14.Shake node off wire: Swing dragging nodes quicklly to bypass this node from wires.  
![14](Resource/1.6/shake_node_off_wire.gif)  

15.Duplicate with inputs:Duplicate selected nodes and keep input wires,default keybind "alt+V"  
![15](Resource/1.6/dupli_node_with_input.gif)  

16.Insert node on wire:  
![16](Resource/1.6/insert_node_on_wire.gif)  

17.Exchange wires:Exchange two sets of wires,default keybind "alt + T".   
![17](Resource/1.6/exchange_wires.gif)  

Go to editor preference to see more settings.  

![14](Resource/1.5/instruction_plugin.png)  
![15](Resource/1.5/instruction_keybind.png)  
![16](Resource/1.5/instruction_config.png)  




 
