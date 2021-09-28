# edpr-vojext

EDPR repository for the European project [VOJEXT](http://vojext.eu/).

## Installation
The software was tested on Linux Pop_OS 20.04 LTS.

- Install [Docker Engine](https://docs.docker.com/engine/install/ubuntu)
- Download the repository and build the Docker image
    ```shell
    $ cd /path/to/repository/folder
    $ docker build -t edpr-vojext:v1 .
    ```


## Usage
- Run the Docker container and, inside it, run the pose detector
    ```shell
    $ xhost +
    $ docker run -it -v /path/to/code/:/code/ -v /path/to/DHP19/dataset/:/data/DHP19/ -v /path/to/checkpoint/:/data/checkpoint/ edpr-vojext:v1 bash
    $ ./launch_pose_detector.sh
    ```

[comment]: <> (  The ``yarpmanager`` window will appear as shown in the figure below.)

[comment]: <> (  ![image]&#40;images/yarpmanager.png&#41;)

[comment]: <> (  The terminal window will show ``yarpserver``'s IP address &#40;``172.17.0.2`` in the figure below&#41;. This might be needed )

[comment]: <> (  for the next step.)

[comment]: <> (  ![image]&#40;images/yarpserver_ip.png&#41;)

[comment]: <> (- Open the script ``launch_yarpview.sh`` and check if line ``11`` has the correct IP address of ``yarpserver`` &#40;gathered)

[comment]: <> (  previously, figure below&#41;.)

[comment]: <> (  If not, set the correct one.)

[comment]: <> (  ![image]&#40;images/yarpview_conf.png&#41;)
  
[comment]: <> (- Run ``yarpview`` on the local machine &#40;yarpview cannot be currently run in the Docker container; this will be fixed in)

[comment]: <> (  a future release&#41;)

[comment]: <> (    ```shell)

[comment]: <> (    $ ./launch_yarpview.sh)

[comment]: <> (    ```)

[comment]: <> (  The script will also download and install locally ``yarp``.)
  
[comment]: <> (- In the ``yarpmanager`` window, open the application menu and select the app "APRIL_WP61a_demo". The list of app )

[comment]: <> (  components will be shown as in the figure above)
 
[comment]: <> (- Run all components by clicking on the green button ``Run all```.)

[comment]: <> (- ``yarpdataplayer``'s GUI will be shown as in the figure below)

[comment]: <> (  ![image]&#40;images/yarpdataplayer.png&#41;)

[comment]: <> (  Select ``File->Open Directory`` and select folder ``data`` as shown in the figure below)

[comment]: <> (  ![image]&#40;images/yarpdataplayer_folder.png&#41;)

[comment]: <> (- Two data sources will be used, the raw events &#40;``ATIS``&#41; and the grayscale frames &#40;``yope_grey``&#41;)

[comment]: <> (  ![image]&#40;images/yarpdataplayer_data.png&#41;)

[comment]: <> (  Select ``Options->Repeat`` and start playing the recorded data by clicking on the ``Play`` button.)

[comment]: <> (- Finally, connect all components by clicking on the green button ``Connect all`` in ``yarpmanager``'s GUI)

[comment]: <> (The previously opened ``yarpview`` should now show the raw events, the grayscale frame and the overlayed 2D skeleton, as)

[comment]: <> (shown in the figure below.)

[comment]: <> (![image]&#40;images/yarpview_gui.png&#41;)