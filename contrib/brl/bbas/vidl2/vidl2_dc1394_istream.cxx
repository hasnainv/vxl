// This is contrib/brl/bbas/vidl2/vidl2_dc1394_istream.cxx
#ifdef VCL_NEEDS_PRAGMA_INTERFACE
#pragma implementation
#endif
//:
// \file
// \author Matt Leotta
// \date   6 Jan 2005
//
//-----------------------------------------------------------------------------

#include "vidl2_dc1394_istream.h"
#include "vidl2_iidc1394_params.h"
#include <vcl_vector.h>

#include <vil/vil_new.h>
#include <vil/vil_image_view.h>


#include <libdc1394/dc1394_control.h>
#include <libdc1394/dc1394_conversions.h>
//#include <libdc1394/dc1394_utils.h>


// used to convert a YUV422 buffer to a RGB8 buffer
static void
vidl2_dc1394_YUV422_to_RGB8(vxl_byte *src, vxl_byte *dest, uint64_t NumPixels)
{
  register int i = (NumPixels << 1)-1; // 2 bytes per pixel
  register int j = NumPixels + ( NumPixels << 1 ) -1; // 3 bytes per pixel
  register int y0, y1, u, v;
  register int r, g, b;

  while (i >= 0) {
    y1 = (vxl_byte) src[i--];
    v  = (vxl_byte) src[i--] - 128;
    y0 = (vxl_byte) src[i--];
    u  = (vxl_byte) src[i--] - 128;
    YUV2RGB (y1, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
    YUV2RGB (y0, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
  }
}


//--------------------------------------------------------------------------------

struct vidl2_dc1394_istream::pimpl
{
  pimpl()
  : vid_index_( unsigned(-1) ),
    camera_info_(NULL),
    max_speed_(DC1394_SPEED_400),
    cur_img_valid_(false)
  {
  }

  //: extract the image properties from the camera mode
  // to initialize cur_img_;
  void init_image();

  unsigned int vid_index_;

  dc1394camera_t* camera_info_;

  int max_speed_;


  //: The last successfully decoded frame.
  mutable vil_image_view<vxl_byte> cur_img_;

  bool cur_img_valid_;

};


//: extract the image properties from the camera mode
// sets ni_, nj_, np_, istep_, jstep_, pstep_
void
vidl2_dc1394_istream::pimpl::init_image()
{
  unsigned int ni, nj, np;
  switch(camera_info_->mode)
  {
    case DC1394_MODE_160x120_YUV444:
      ni=160; nj=120; np=3;
      break;
    case DC1394_MODE_320x240_YUV422:
      ni=320; nj=240; np=3;
      break;
    case DC1394_MODE_640x480_YUV411:
    case DC1394_MODE_640x480_YUV422:
    case DC1394_MODE_640x480_RGB8:
      ni=640; nj=480; np=3;
      break;
    case DC1394_MODE_640x480_MONO8:
      ni=640; nj=480; np=1;
      break;
    case DC1394_MODE_800x600_YUV422:
    case DC1394_MODE_800x600_RGB8:
      ni=800; nj=600; np=3;
      break;
    case DC1394_MODE_800x600_MONO8:
      ni=800; nj=600; np=1;
      break;
    case DC1394_MODE_1024x768_YUV422:
    case DC1394_MODE_1024x768_RGB8:
      ni=1024; nj=768; np=3;
      break;
    case DC1394_MODE_1024x768_MONO8:
      ni=1024; nj=768; np=1;
      break;
    case DC1394_MODE_1280x960_YUV422:
    case DC1394_MODE_1280x960_RGB8:
      ni=1280; nj=960; np=3;
      break;
    case DC1394_MODE_1280x960_MONO8:
      ni=1280; nj=960; np=1;
      break;
    case DC1394_MODE_1600x1200_YUV422:
    case DC1394_MODE_1600x1200_RGB8:
      ni=1600; nj=1200; np=3;
      break;
    case DC1394_MODE_1600x1200_MONO8:
      ni=1600; nj=1200; np=1;
      break;
    default:
      vcl_cerr << "camera mode not currently supported by vidl2" << vcl_endl;
  }

  cur_img_ = vil_image_view<vxl_byte>(ni, nj, 1, np);
  vcl_cout << "image size: "<<ni<<"x"<<nj<<"x"<<np<<vcl_endl;
}

//--------------------------------------------------------------------------------

//: Constructor
vidl2_dc1394_istream::
vidl2_dc1394_istream()
  : is_( new vidl2_dc1394_istream::pimpl )
{
}



//: Destructor
vidl2_dc1394_istream::
~vidl2_dc1394_istream()
{
  close();
  delete is_;
}

//: Open a new stream using a filename
bool
vidl2_dc1394_istream::
open(const vidl2_iidc1394_params& params)
{
  // Close any currently opened file
  close();

  is_->camera_info_ = dc1394_new_camera(0,0);//(params.port, params.node);

  if(dc1394_get_camera_info(is_->camera_info_) != DC1394_SUCCESS){
    close();
    return false;
  }

  if(dc1394_video_set_iso_channel_and_speed(is_->camera_info_, 0, params.speed_) != DC1394_SUCCESS){
    vcl_cerr << "Failed to set iso channel and speed.\n";
    close();
    return false;
  }

  if(dc1394_video_set_mode(is_->camera_info_, params.video_mode_) != DC1394_SUCCESS){
    vcl_cerr << "Failed to set video mode.\n";
    close();
    return false;
  }
  else
    is_->camera_info_->mode = params.video_mode_;

  if(dc1394_video_set_framerate(is_->camera_info_, params.frame_rate_) != DC1394_SUCCESS){
    vcl_cerr << "Failed to set frame rate.\n";
    close();
    return false;
  }
  else
    is_->camera_info_->framerate = params.frame_rate_;

  if (!((is_->camera_info_->mode >= DC1394_MODE_FORMAT7_MIN) &&
         (is_->camera_info_->mode <= DC1394_MODE_FORMAT7_MAX))) {
    int err=dc1394_setup_capture( is_->camera_info_, is_->camera_info_->iso_channel,
                                  is_->camera_info_->mode, is_->max_speed_,
                                  is_->camera_info_->framerate );

    if (err!=DC1394_SUCCESS){
      vcl_cerr << "Failed to setup RAW1394 capture. Error code "<< err << '\n';
      return false;
    }

  }

  is_->init_image();


  // turn on the camera power
  dc1394switch_t pwr;
  if(dc1394_video_get_transmission(is_->camera_info_, &pwr) == DC1394_SUCCESS){
    if(pwr == DC1394_ON ){
      dc1394_video_set_transmission(is_->camera_info_, DC1394_OFF);
      vcl_cout << "power already on" << vcl_endl;
    }
    if(dc1394_video_set_transmission(is_->camera_info_, DC1394_ON) == DC1394_SUCCESS) {
      vcl_cout << "power turned on" << vcl_endl;
    }
    return true;
  }
  else{
    vcl_cerr << "unable to start camera iso transmission\n";
    close();
    return false;
  }

  return true;
}


//: Close the stream
void
vidl2_dc1394_istream::
close()
{
  if(is_->camera_info_){
    // turn off the camera power
    dc1394switch_t pwr;
    if(dc1394_video_get_transmission(is_->camera_info_, &pwr) == DC1394_SUCCESS &&
       pwr == DC1394_ON){
      dc1394_video_set_transmission(is_->camera_info_, DC1394_OFF);
    }

    dc1394_release_camera(is_->camera_info_);
    dc1394_free_camera(is_->camera_info_);
    is_->camera_info_ = NULL;
  }
  is_->vid_index_ = unsigned(-1);
}


//: Probe the bus to determine the valid parameter options
bool
vidl2_dc1394_istream::
valid_params(vidl2_iidc1394_params::valid_options& options)
{
  dc1394camera_t **dccameras=NULL;
  unsigned int camnum=0;

  // find the cameras using classic libdc functions:
  unsigned int err = dc1394_find_cameras(&dccameras,&camnum);

  if(err == DC1394_NO_CAMERA){
    options.cameras.clear();
    return true;
  }
  else if(err){
    vcl_cerr << "error finding cameras, error code: " << err << vcl_endl;
    return false;
  }

  options.cameras.resize(camnum);

  // create a list of cameras 
  for (unsigned int i=0; i<camnum; ++i) {
    options.cameras[i].node = dccameras[i]->node;
    options.cameras[i].port = dccameras[i]->port;
    options.cameras[i].vendor = dccameras[i]->vendor;
    options.cameras[i].model = dccameras[i]->model;

    dc1394videomodes_t modes;
    if( dc1394_video_get_supported_modes(dccameras[i], &modes ) <0 ){
      vcl_cerr << "Could not find any supported video modes\n";
      return false;
    }
    //const vidl2_iidc1394_params::valid_options::valid_modes& m = 
    options.cameras[i].modes.resize(modes.num);
    for(unsigned int j=0; j<modes.num; ++j){
      options.cameras[i].modes[j].mode = (vidl2_iidc1394_params::video_mode_t) modes.modes[j];
      unsigned int format = vidl2_iidc1394_params::video_format_val(options.cameras[i].modes[j].mode);
      if(format > 5)
        continue;
      dc1394framerates_t framerates;
      dc1394_video_get_supported_framerates(dccameras[i], modes.modes[j], &framerates);
      options.cameras[i].modes[j].frame_rates.resize(framerates.num);
      for(unsigned int k=0; k<framerates.num; ++k){
        options.cameras[i].modes[j].frame_rates[k] = (vidl2_iidc1394_params::frame_rate_t)framerates.framerates[k];
      }
    }
  }


  if(camnum > 0){
    // free the cameras
    for (unsigned int i=0; i<camnum; ++i)
      free(dccameras[i]);

    free(dccameras);
  }

  return true;
}


//: Return true if the stream is open for reading
bool
vidl2_dc1394_istream::
is_open() const
{
  return bool(is_->camera_info_);
}


//: Return true if the stream is in a valid state
bool 
vidl2_dc1394_istream::
is_valid() const
{
  return is_open();
}


//: Return true if the stream support seeking
bool
vidl2_dc1394_istream::
is_seekable() const
{
  return false;
}


//: Return the current frame number
unsigned int 
vidl2_dc1394_istream::
frame_number() const
{
  return is_->vid_index_;
}


//: Advance to the next frame (but don't acquire an image)
bool
vidl2_dc1394_istream::
advance()
{
  ++is_->vid_index_;
  is_->cur_img_valid_ = false;
  if(dc1394_capture(&is_->camera_info_, 1) != DC1394_SUCCESS){
    vcl_cerr << "capture failed" << vcl_endl;
    return false;
  }
  return true;
}


//: Read the next frame from the stream
vil_image_resource_sptr
vidl2_dc1394_istream::read_frame()
{
  if(advance())
    return current_frame();
  return NULL;
}


//: Return the current frame in the stream
vil_image_resource_sptr
vidl2_dc1394_istream::current_frame()
{
  // Quick return if the stream isn't valid
  if ( !is_valid() ) {
    return NULL;
  }

#if 0
  is_->cur_img_.set_to_memory((vxl_byte*)is_->camera_info_->capture.capture_buffer,
                              is_->cur_img_.ni(), is_->cur_img_.nj(), is_->cur_img_.nplanes(),
                              is_->cur_img_.istep(), is_->cur_img_.jstep(), is_->cur_img_.planestep());
#endif

  if(!is_->cur_img_valid_){
    vidl2_dc1394_YUV422_to_RGB8((vxl_byte*)is_->camera_info_->capture.capture_buffer,
                                is_->cur_img_.top_left_ptr(),
                                is_->cur_img_.ni() * is_->cur_img_.nj());
    is_->cur_img_valid_ = true;
  }


  if(is_->cur_img_)
    return vil_new_image_resource_of_view(is_->cur_img_);
  return NULL;
}


//: Seek to the given frame number
// \returns true if successful
bool
vidl2_dc1394_istream::
seek_frame(unsigned int frame)
{
  return false;
}

