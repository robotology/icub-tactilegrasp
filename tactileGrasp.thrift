#tactileGrasp.thrift


/**
 * tactileGrasp_IDLServer
 *
 * IDL Interface to \ref tactileGrasp services.
 */
service tactileGrasp_IDLServer
{
    /**
     * Opens the robot hand.
     * @return true/false on success/failure
     */
    bool open();

    /**
     * Grasp an object using feedback from the fingertips tactile sensors.
     * The grasping movement is stoped upon contact detection.
     * @return true/false on success/failure.
     */
    bool grasp();

    /**
     * Grasp object without using the feedback from the fingertips tactile sensors.
     * The grasping movement is not controlled and the object is therefore crushed.
     * @return true/false on success/failure.
     */
    bool crush();

    /**
     * Quit the module.
     * @return true/false on success/failure.
     */
    bool quit();  
}
