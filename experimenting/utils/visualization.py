"""
Visualization toolbox
"""
from matplotlib import pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from experimenting.utils.skeleton_helpers import Skeleton
import numpy as np

def _get_3d_ax():
    fig = plt.figure(figsize=(8, 8))
    ax = Axes3D(fig)
    ax.xaxis.pane.fill = False
    ax.yaxis.pane.fill = False
    ax.zaxis.pane.fill = False

    # Now set color to white (or whatever is "invisible")
    ax.xaxis.pane.set_edgecolor('w')
    ax.yaxis.pane.set_edgecolor('w')
    ax.zaxis.pane.set_edgecolor('w')

    # Bonus: To get rid of the grid as well:
    ax.grid(False)
    ax.view_init(30, 240)
    return ax


def plot_heatmap(img):
    fig, ax = plt.subplots(ncols=img.shape[0], nrows=1, figsize=(20, 20))
    for i in range(img.shape[0]):
        ax[i].imshow(img[i])
        ax[i].axis('off')
    plt.show()


def plot_skeleton_3d(skeleton_gt, skeleton_pred=None, fname=None):
    """
        Args:
           M: extrinsic matrix as tensor of shape 4x3
           xyz: torch tensor of shape NUM_JOINTSx3
           pred: torch tensor of shape NUM_JOINTSx3
        """

    ax = _get_3d_ax()
    skeleton_gt.plot_3d(ax, c='red')
    if skeleton_pred is not None:
        skeleton_pred.plot_3d(ax, c='blue')
    if fname is not None:
        plt.savefig(fname)


def plot_2d_from_3d(dvs_frame, gt_skeleton, p_mat, pred_skeleton=None):
    """
        To plot image and 2D ground truth and prediction

        Args:
          dvs_frame: frame as vector (1xWxH)
          sample_gt: gt joints as vector (N_jointsx2)

        """

    fig = plt.figure()

    ax = fig.add_axes([0, 0, 1, 1])
    ax.axis('off')
    ax.imshow(dvs_frame)
    H, W = dvs_frame.shape

    gt_joints = gt_skeleton.get_2d_points(p_mat, 346, 260)
    ax.plot(gt_joints[:, 0], gt_joints[:, 1], '.', c='red', label='gt')
    if pred_skeleton is not None:
        pred_joints = pred_skeleton.get_2d_points(p_mat, 346, 260)
        ax.plot(pred_joints[:, 0], pred_joints[:, 1], '.', c='blue', label='pred')

    plt.legend()


def plot_skeleton_2d(dvs_frame, gt_joints, pred_joints=None,fname=None, return_figure=False, lines = False):
    """
        To plot image and 2D ground truth and prediction

        Args:
          dvs_frame: frame as vector (1xWxH)
          sample_gt: gt joints as vector (N_jointsx2)

        """

    fig = plt.figure()
    ax = fig.add_axes([0, 0, 1, 1])
    ax.imshow(dvs_frame)
    ax.axis('off')
    H, W = dvs_frame.shape
    ax.plot(gt_joints[:, 0], H-gt_joints[:, 1], '.', c='red')
    if lines:
        plot_2d(ax, gt_joints[:, 0], H - gt_joints[:, 1], c='red')
    if pred_joints is not None:
        ax.plot(pred_joints[:, 0], H-pred_joints[:, 1], '.', c='blue')
        if lines:
            plot_2d(ax, pred_joints[:, 0], H - pred_joints[:, 1], c='blue')
    plt.legend()
    if fname is not None:
        plt.savefig(fname)
        return None
    if return_figure:
        return fig

def plot_skeleton_2d_lined(dvs_frame, gt_joints=None, pred_joints=None,fname=None, return_figure=False):
    fig = plot_skeleton_2d(dvs_frame, gt_joints=gt_joints pred_joints=pred_joints, fname=fname, return_figure=return_figure, lines=True)
    if return_figure:
        return fig


def plot_2d(ax, x, y, c="red", limits=None, plot_lines=True):
    """
    Plot the provided skeletons in 2D coordinate space
    Args:
      ax: axis for plot
      y_true_pred: joints to plot in 2D coordinate space
      c: color (Default value = 'red')
      limits: list of 3 ranges (x, y, and z limits)
      plot_lines:  (Default value = True)

    Note:
      Plot the provided skeletons. Visualization purpose only

    From DHP19 toolbox - modified
    """

    if limits is None:
        limits = [[-500, 500], [-500, 500]]

    # points = self._get_tensor()
    # x = points[:, 0]
    # y = points[:, 1]

    # ax.scatter(x, y, s=20, c=c, marker="o", depthshade=True)

    lines_skeleton = Skeleton._get_skeleton_lines_2D(x, y)

    if plot_lines:
        for line in range(len(lines_skeleton)):
            ax.plot(
                lines_skeleton[line, 0, :],
                lines_skeleton[line, 1, :],
                c,
                label="gt",
            )

    ax.set_xlabel("X Label")
    ax.set_ylabel("Y Label")
    x_limits = limits[0]
    y_limits = limits[1]
    x_range = np.abs(x_limits[1] - x_limits[0])
    x_middle = np.mean(x_limits)
    y_range = np.abs(y_limits[1] - y_limits[0])
    y_middle = np.mean(y_limits)
    # The plot bounding box is a sphere in the sense of the infinity
    # norm, hence I call half the max range the plot radius.
    # plot_radius = 0.5 * np.max([x_range, y_range])
    # ax.set_xlim3d([x_middle - plot_radius, x_middle + plot_radius])
    # ax.set_ylim3d([y_middle - plot_radius, y_middle + plot_radius])
