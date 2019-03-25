/*
** Copyright (C) 2004 Yuwu (Bruin) Xiong <xiongyw@hotmail.com>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

////////////////////////////////////////////////////////////////////////////
// changelog
//
//   2003-02-01: first creation, support MSIE only
//   2004-07-08: support mozilla, cleanup code, and add code comments
////////////////////////////////////////////////////////////////////////////


/* constructor of node object: Node(id, desc, type, href)

   id: unique id of node objects. using the memory address
       (eg, "0807d4a0") as the id to achive the uniqueness. 

       other objects' name comforms to the following convention:
      "N" + id: the node object (eg, N0807d4a0 is the node 
                object whose id is "0807d4a0"
      "D" + id: the DIV object of the node (yes, each node has 
                a DIV object in html document)
      "J" + id: the node's joint icon image object in html document. 
                the joint icon is to represent the status of the 
                node in browser. there are 6 possible status for a node: 
                
              |          |         |         |         |        |
            +---+      +---+       |       +---+     +---+      |
            | + |--    | - |--     +----   | + |--   | - |--    +----
            +---+      +---+       |       +---+     +---+
              |          |         |         
  
             (a)        (b)       (c)       (d)       (e)       (f)
                                                             
             (a) a closed node having kid(s)  
             (b) an opened/collapsed node having kid(s)
             (c) a leaf node having no kids
             (d) a last closed node having kid(s)
             (e) a last opened/collapsed node having kid(s)
             (f) a last leaf node having no kids
                
             Clicking on nodes cause the node to open/close, so the 
             joint icon should also change accordingly.

      "P" + id: packet hex view html file for packet node 
                (eg, "./packets/P0807d4a0.html")
      "S" + id: section hex view html file for section node 
                (eg, "./sections/S0807d4a0.html")
      
   desc: node text description
   type: node type, each type has an icon. node types are TS/TABLE/etc
   href: link page (optional, for packet and section node only)
*/   
function Node(id, desc, type, href){

    // tree's data structure
    this.dad = null;
    this.kids = [];

    // static properties assigned
    this.id = id;
    this.desc = desc;
    this.type = type;
    this.href = href;

    // properties determined during tree_init()
    this.left_code = "";
    this.is_last = false;  // is the last kid of its parent?
    
    // dynamic properties
    this.is_open = false;  // is collapsed or not? 
    
    // methods
    this.add_kid         = _add_kid;
    this.set_left_n_last = _set_left_n_last;
    this.get_left_icons  = _get_left_icons;  
    this.get_jicon_src   = _get_jicon_src;   
    this.get_type_icon   = _get_type_icon;   
    this.render          = _render;          
    this.onclick         = _onclick;         
}

function _add_kid(kid){
    this.kids[this.kids.length] = kid;
    kid.dad = this;
    return kid;
}

/* recursively set "left_code" and "is_last" properties
   of each node in the tree. called by the root of the tree
   after the tree structure been built in memory 
*/
function _set_left_n_last(is_last, left_code){
    var i;
    
    this.is_last = is_last;
    this.left_code = left_code;
    
    if(is_last){
        left_code += "0";   
    }
    else{
        left_code += "1";   
    }
    
    for(i = 0; i < this.kids.length; i ++){
    	if(i == this.kids.length - 1){
            this.kids[i].set_left_n_last(true, left_code);
        }
        else{
            this.kids[i].set_left_n_last(false, left_code);
        }
    }
}

/* generate html elements of the node. for each node, its html elements 
   could be divided into 5 parts:
   1. left side icons: a series of blank and/or vertline icons
   2. joint icon: one of 6 icons as described above 
   3. type icon: icon representing the type of the node
   4. description text: plain text or anchored text
   5. div object: the container of child nodes' html elements. this is 
        the only part which supports show/hide. showing div of a node 
        opens/collapses the node, while hiding div of the node closes
        the node.
*/
function _render(){ 

    var docW = "";

    docW += "<table border=0 cellspacing=0 cellpadding=0><tr>";

    // 1. left side icons
    docW += this.get_left_icons();

    // 2. joint icon 
    if(this.dad){    // do not show joint icon for top level node(s)
        if(this.kids.length > 0)
            docW += "<td nowrap><a href='javascript: N" + this.id + ".onclick()'><img name='J" + this.id + "' src='" + this.get_jicon_src() + "' align=absbottom width=16 height=16 border=0></a></td>";
        else
            docW += "<td nowrap><img name='J" + this.id + "' src='" + this.get_jicon_src() + "' align=absbottom width=16 height=16 border=0></td>";
    }

    // 3. type icon
    docW += this.get_type_icon();

    // 4. desc text
    docW += "<td valign=middle nowrap width=100%>&nbsp;";
    if(this.type == 5) // section 
        docW += "<a href='./sections/S" + this.id + ".html' target='hex_frame'>";
    if(this.type == 13) // packet
        docW += "<a href='./packets/P" + this.id + ".html' target='hex_frame'>";
    docW += this.desc; 
    if(this.type == 5 || this.type == 13)
        docW += "</a>";
    docW += "</td></tr></table>";

	// 5. div object
    docW += "<div id='D" + this.id + "' style='display: none'></div>";
    
    return docW;
} 

/* toggle betw. open & close of the node
*/
function _onclick(){
    var div = get_element("D" + this.id);
    if(!div)  return;
    if(!div.innerHTML){
    	var kids = [];
    	for(var i = 0; i < this.kids.length; i ++){
    		kids[i] = this.kids[i].render();
	}
	div.innerHTML = kids.join('');
    }

    // toggle visibility of its kids
    if(this.is_open == false){
    	div.style.display = "block";
    	this.is_open = true;
    }
    else{
        div.style.display = "none";
        this.is_open = false;
    }
    
    // toggle the joint icon
    // note: get_element() does not work on mozilla, while document.images[] works
    //       on both mozilla and ie.
    //var jicon = get_element("J" + this.id);
    var jicon = document.images["J" + this.id];
    if(jicon) jicon.src = this.get_jicon_src();    
}

/* get left side icons of a node (componsed by blank and/or vertline) */ 
function _get_left_icons(){
    var left_array = [];
    for(var i = 1; i < this.left_code.length; i ++){
        if(this.left_code.charAt(i) == "1")
            left_array[i] = "<td nowrap><img src='./images/vertline.gif' align=absbottom width=16 height=16 border=0></td>";
        else
            left_array[i] = "<td nowrap><img src='./images/blank.gif' align=absbottom width=16 height=16 border=0></td>";
    }
    return left_array.join('');
}

/* get joint icon src */
function _get_jicon_src(){
	
    if(this.dad == null){
        return "./images/blank.gif";
    }
    else{
        if(this.is_last){
            if(this.kids.length > 0){
                if(this.is_open)
                    return "./images/mlastnode.gif";
                else
                    return "./images/plastnode.gif";
            }
            else{
                return "./images/lastnode.gif";
            }
        }
        else{ // not last node 
            if(this.kids.length > 0){
                if(this.is_open)
                    return "./images/mnode.gif";
                else
                    return "./images/pnode.gif";
            }
            else{
                return "./images/node.gif";
            }
        }
    }
}

/* get the type icon of the node, according to its type */
function _get_type_icon(){
	
    var img = "<td nowrap><img src='"; 
    
    switch(this.type){
        case  1:    img += "./images/01.gif"; break;
        case  2:    img += "./images/02.gif"; break;
        case  3:    img += "./images/03.gif"; break;
        case  4:    img += "./images/04.gif"; break;
        case  5:    img += "./images/05.gif"; break;
        case  6:    img += "./images/06.gif"; break;
        case  7:    img += "./images/07.gif"; break;
        case  8:    img += "./images/08.gif"; break;
        case  9:    img += "./images/09.gif"; break;
        case 10:    img += "./images/10.gif"; break;
        case 11:    img += "./images/11.gif"; break;
        case 12:    img += "./images/12.gif"; break;
        case 13:    img += "./images/13.gif"; break;
        default:    img += "./images/00.gif"; break;
    }
    img += "' align=absbottom width=16 height=16 border=0></td>";
    return img;
}


function tree_init(root){
    root.set_left_n_last(true, "");
    
    document.write(root.render());
    root.onclick();    
}

function get_element(id){
    return document.all? document.all(id) : document.getElementById(id);
}

