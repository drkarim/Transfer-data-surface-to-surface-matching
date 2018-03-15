# TransferScalar

A VTK utility program that will copy point data scalars between two VTK polygonal data meshes. 

For copying the scalar data, it has two options: 

- Direct copy using corresponding point IDs 
- Shortest distance between points of the target and source meshes 

Note that the two surfaces source and target must have exact same number of vertices or points
Also, note that the ICP function does not work and it has bugs 

# Usage 

```
Required parameters: 
	-i1 source 
	-i2 target 
	-o output 
	--directcopy  
	--icp x (does not work)

```
