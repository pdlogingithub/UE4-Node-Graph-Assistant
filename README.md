# UE4-Node-Graph-Assistant User Guide

Marketplace page: https://www.unrealengine.com/marketplace/node-graph-assistant  
Forum support page: https://forums.unrealengine.com/unreal-engine/marketplace/1435240-node-graph-assistant  

中文说明：https://github.com/pdlogingithub/UE4-Node-Graph-Assistant/blob/master/Doc/README_ch.md  

1. Right click multi-connect: right click on pin to multi-connect,right click on panel to cancel,you can also pan the graph when dragging a wire.  
![1](Resource/1.4/drag_pan_multi-connect.gif)

2. Left click multi-connect: Click on pin to start free panning,zooming and multi-connecting.  
![2](Resource/1.4/click_pan_multi-connect.gif)

3. Shift click multi-connect: When dragging a wire, hold down shift to start free panning,zooming and multi-connecting.  
![3](Resource/1.4/shift_pan_multi-connect.gif)

4. Dupli-wire: Shift click or drag on pin to duplicate wire.  
![4](Resource/1.4/dupli_wire.gif)

5. Rearrange nodes: Press <kbd>Alt</kbd>+<kbd>R</kbd> to rearrange nodes, most suitable for small block of nodes.   
![5](Resource/1.4/rearrange.gif)

6.Connect nodes: Press <kbd>Alt</kbd>+<kbd>C</kbd> to auto connect all plausible pins withing selected nodes.  
![6](Resource/connectnodes.gif)  

7. Bypass node: Press <kbd>Alt</kbd>+<kbd>X</kbd> will remove selected nodes on wire.  
*additionally hold down <kbd>Shift</kbd> will keep bypassed nodes in place without removing them.  
*additionally config in editor settings allow bypassed nodes to be put into clipboard for later paste action.  
![7](Resource/1.4/bypass.gif)

8. Create node on wire: Right click on wire to create node on wire.  
*also works for nodes created with hotkeys,for example,in material editor hold down "a" key and click on wire will create a add node on wire.  
![8](Resource/1.4/insert.gif)
![81](Resource/insertnodehotkey.gif)


9. Highlight wire: Left click to highlight wire, hold down shift to highlight multiple wires, middle mouse double click on pin to highlight all connected wires.  
![9](Resource/1.4/highlight.gif)

10. Cutoff wires: Hold <kbd>Alt</kbd> and drag across wires to break them.  
![10](Resource/1.4/cutoff.gif)

11. Select linked nodes: Middle mouse double click on a node to select all connected nodes.  
Depending on whether the left or right area of the node is clicked on, this will select all children or all parents.  
Or use the hotkey <kbd>Alt</kbd>+<kbd>A</kbd>, or <kbd>Alt</kbd>+<kbd>D</kbd>.  
![11](Resource/1.4/select_linked.gif)

12. Lazy connect: When dragging off of a pin, the wire will snap to the closest pin on the hovered node.  
(Hold <kbd>Shift</kbd> and dragging over pins to batch connect.it may appears laggy if "live preview" button is on for material graph.) 
 
 ![12](Resource/1.5/lazy_connect.gif)
 
13. Auto connect: When moving a node, its pins will align to surrounding connectible pins. Releasing mouse will commit connections.  
Holding <kbd>Alt</kbd> will suppress auto connect.   
![13](Resource/1.5/auto_connect.gif)

14. Wire style: press the toolbar button to cycle through wire styles.  
![14](Resource/1.5/wire_style.gif)

15. Shake node off wire: Shake a node quickly while dragging to bypass this node from wires.  
![15](Resource/1.6/shake_node_off_wire.gif)

16. Duplicate with inputs: Duplicate selected nodes and keep input wires with <kbd>Alt</kbd>+<kbd>V</kbd>  
![16](Resource/1.6/dupli_node_with_input.gif)

17. Insert node on wire:  
![17](Resource/1.6/insert_node_on_wire.gif)

18.  Exchange wires: Exchange two sets of wires with <kbd>Alt</kbd>+<kbd>T</kbd>  
*cursor position will direct which wires to operate on if there are multiple plausible wire sets.  
![18](Resource/1.6/exchange_wires.gif)

Go to the editor preferences to see more settings.

![14](Resource/1.5/instruction_plugin.png)  
![15](Resource/1.5/instruction_keybind.png)  
![16](Resource/1.5/instruction_config.png)  
