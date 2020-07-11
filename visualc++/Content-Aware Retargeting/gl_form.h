#pragma once

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <GL\glew.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <opencv\cv.hpp>

#include "gl_mesh.h"
#include "gl_texture.h"
#include "saliency.h"
#include "segmentation.h"
#include "triangle.h"
#include "video_segmentation.h"
#include "warping.h"

#include <msclr\marshal_cppstd.h>
#using <mscorlib.dll>

namespace ContentAwareRetargeting {

  enum ProgramStatus {
    IDLE,
    IMAGE_RETARGETING,
    VIDEO_RETARGETING,
    IMAGE_FOCUS
  };

  ProgramStatus program_status;

  GLShader gl_shader;

  const float FOVY = 45.0f;

  glm::vec3 eye_position(0.0, 0.0, 0.0);
  glm::vec3 look_at_position(0.0, 0.0, 0.0);

  const float EYE_POSITION_SCALE_PER_SCROLLING = 0.9f;
  float eye_position_scale = 1.0;

  bool is_dragging_panel;

  const int MIN_PANEL_WIDTH = 10;
  const int MIN_PANEL_HEIGHT = 10;
  const int MAX_PANEL_WIDTH = 1 << 12;
  const int MAX_PANEL_HEIGHT = 1 << 12;

  auto last_clock = std::chrono::high_resolution_clock::now();

  std::string source_image_file_directory;
  std::string source_image_file_name;

  cv::Mat source_image;
  cv::Mat source_image_after_smoothing;
  cv::Mat source_image_after_segmentation;
  cv::Mat saliency_map_of_source_image;
  cv::Mat significance_map_of_source_image;

  std::string source_video_file_directory;
  std::string source_video_file_name;

  std::vector<cv::Mat> source_video_frames;
  std::vector<cv::Mat> source_video_frames_after_segmentation;

  double source_video_fps;
  int source_video_fourcc;

  GLMesh gl_panel_image_mesh;

  GLMesh gl_panel_image_triangle_mesh;

  const float GRID_LINE_WIDTH = 4.5;
  const float GRID_POINT_SIZE = 7.5;

  float current_grid_size = 50;

  const float MIN_GRID_SIZE_WHEN_FOCUS = 50;

  double focus_grid_scale_modifier = 0.234;
  double focus_position_x;
  double focus_position_y;

  bool is_viewing_mesh = true;
  bool is_viewing_mesh_point = false;

  bool is_demo_triangle = false;

  Graph<glm::vec2> image_graph;
  std::vector<std::vector<int> > group_of_pixel;
  std::vector<std::vector<double> > saliency_map;
  std::vector<double> saliency_of_patch;
  std::vector<double> saliency_of_mesh_vertex;

  std::map<size_t, double> saliency_of_object;

  bool data_for_image_warping_were_generated = false;

  bool is_recording_screen = false;

  using namespace System::Runtime::InteropServices;

  [DllImport("opengl32.dll")]
  extern HDC wglSwapBuffers(HDC hdc);

  using namespace System;
  using namespace System::ComponentModel;
  using namespace System::Collections;
  using namespace System::Windows::Forms;
  using namespace System::Data;
  using namespace System::Drawing;

  /// <summary>
  /// Summary for GLForm
  /// </summary>
  public ref class GLForm : public System::Windows::Forms::Form {

  public:

    GLForm();

  private:

    HWND hwnd;
    HDC hdc;
    HGLRC hrc;

    void InitializeOpenGL();

    void RenderGLPanel();

    void ChangeProgramStatus(const ProgramStatus &new_program_status);

    void SaveGLScreen(const std::string &file_path);

    void SaveGLScreenToVideo(cv::VideoWriter &video_writer);

    void ChangeGLPanelSize(int new_panel_width, int new_panel_height);

    void BuildGridMeshAndGraphForImage(const cv::Mat &image, GLMesh &target_mesh, Graph<glm::vec2> &G, float grid_size);

    void BuildTriangleMesh(const cv::Mat &image, GLMesh &target_mesh, const float triangle_size);

    void GenerateDataForImageWarping();

    void ContentAwareImageRetargeting(const int target_image_width, const int target_image_height, const float grid_size);

    void ContentAwareVideoRetargetingUsingObjectPreservingWarping(const int target_video_width, const int target_video_height, const float grid_size);

    void ImageFocusWarping(const int target_image_width, const int target_image_height, const float grid_size);

    cv::Vec3b SaliencyValueToSignifanceColor(double saliency_value);

    size_t Vec3bToValue(const cv::Vec3b &color);

    void OnButtonsClick(System::Object ^sender, System::EventArgs ^e);

    void OnMouseDown(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e);

    void OnMouseUp(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e);

    void OnMouseMove(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e);

    void OnKeyDown(System::Object ^sender, System::Windows::Forms::KeyEventArgs ^e);

    void OnTrackBarsValueChanged(System::Object ^sender, System::EventArgs ^e);

    void OnCheckBoxesCheckedChanged(System::Object ^sender, System::EventArgs ^e);

    void OnNumericUpDownValueChanged(System::Object ^sender, System::EventArgs ^e);

  protected:
    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    ~GLForm() {
      if (components) {
        delete components;
      }
    }

  private:

    System::ComponentModel::Container ^components;
    System::Windows::Forms::Panel ^gl_panel_;
    System::Windows::Forms::MenuStrip ^menu_strip_;
    System::Windows::Forms::ToolStripMenuItem ^file_tool_strip_menu_item_;
    System::Windows::Forms::ToolStripMenuItem ^mode_tool_strip_menu_item_;
    System::Windows::Forms::ToolStripMenuItem ^view_tool_strip_menu_item_;
    System::Windows::Forms::ToolStripMenuItem ^open_image_file_tool_strip_menu_item_;
    System::Windows::Forms::ToolStripMenuItem ^open_video_file_tool_strip_menu_item_;
    System::Windows::Forms::ToolStripMenuItem ^save_screen_tool_strip_menu_item_;
    System::Windows::Forms::ToolStripMenuItem ^record_screen_tool_strip_menu_item_;
    System::Windows::Forms::TrackBar ^grid_size_track_bar_;
    System::Windows::Forms::TrackBar ^focus_scale_track_bar_;
    System::Windows::Forms::Label ^grid_size_label_;
    System::Windows::Forms::Label ^focus_scale_label_;
    System::Windows::Forms::Label ^original_size_label_;
    System::Windows::Forms::Label ^target_size_label_;
    System::Windows::Forms::Button ^start_button_;
    System::Windows::Forms::CheckBox ^show_lines_check_box_;
    System::Windows::Forms::CheckBox ^show_image_check_box_;
    System::Windows::Forms::CheckBox ^focus_check_box_;
    System::Windows::Forms::NumericUpDown ^target_width_numeric_up_down_;
    System::Windows::Forms::NumericUpDown ^target_height_numeric_up_down_;

#pragma region Windows Form Designer generated code
    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    void InitializeComponent(void) {
      this->gl_panel_ = (gcnew System::Windows::Forms::Panel());
      this->menu_strip_ = (gcnew System::Windows::Forms::MenuStrip());
      this->file_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->open_image_file_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->open_video_file_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->save_screen_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->record_screen_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->mode_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->view_tool_strip_menu_item_ = (gcnew System::Windows::Forms::ToolStripMenuItem());
      this->grid_size_track_bar_ = (gcnew System::Windows::Forms::TrackBar());
      this->grid_size_label_ = (gcnew System::Windows::Forms::Label());
      this->original_size_label_ = (gcnew System::Windows::Forms::Label());
      this->target_size_label_ = (gcnew System::Windows::Forms::Label());
      this->start_button_ = (gcnew System::Windows::Forms::Button());
      this->show_lines_check_box_ = (gcnew System::Windows::Forms::CheckBox());
      this->show_image_check_box_ = (gcnew System::Windows::Forms::CheckBox());
      this->target_width_numeric_up_down_ = (gcnew System::Windows::Forms::NumericUpDown());
      this->target_height_numeric_up_down_ = (gcnew System::Windows::Forms::NumericUpDown());
      this->focus_scale_label_ = (gcnew System::Windows::Forms::Label());
      this->focus_scale_track_bar_ = (gcnew System::Windows::Forms::TrackBar());
      this->focus_check_box_ = (gcnew System::Windows::Forms::CheckBox());
      this->menu_strip_->SuspendLayout();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->grid_size_track_bar_))->BeginInit();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->target_width_numeric_up_down_))->BeginInit();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->target_height_numeric_up_down_))->BeginInit();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->focus_scale_track_bar_))->BeginInit();
      this->SuspendLayout();
      // 
      // gl_panel_
      // 
      this->gl_panel_->BackColor = System::Drawing::SystemColors::Desktop;
      this->gl_panel_->Cursor = System::Windows::Forms::Cursors::Default;
      this->gl_panel_->Location = System::Drawing::Point(200, 50);
      this->gl_panel_->Name = L"gl_panel_";
      this->gl_panel_->Size = System::Drawing::Size(800, 600);
      this->gl_panel_->TabIndex = 0;
      // 
      // menu_strip_
      // 
      this->menu_strip_->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {
        this->file_tool_strip_menu_item_,
          this->mode_tool_strip_menu_item_, this->view_tool_strip_menu_item_
      });
      this->menu_strip_->Location = System::Drawing::Point(0, 0);
      this->menu_strip_->Name = L"menu_strip_";
      this->menu_strip_->Size = System::Drawing::Size(1084, 24);
      this->menu_strip_->TabIndex = 1;
      this->menu_strip_->Text = L"Menu";
      // 
      // file_tool_strip_menu_item_
      // 
      this->file_tool_strip_menu_item_->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(4) {
        this->open_image_file_tool_strip_menu_item_,
          this->open_video_file_tool_strip_menu_item_, this->save_screen_tool_strip_menu_item_, this->record_screen_tool_strip_menu_item_
      });
      this->file_tool_strip_menu_item_->Name = L"file_tool_strip_menu_item_";
      this->file_tool_strip_menu_item_->Size = System::Drawing::Size(37, 20);
      this->file_tool_strip_menu_item_->Text = L"File";
      // 
      // open_image_file_tool_strip_menu_item_
      // 
      this->open_image_file_tool_strip_menu_item_->Name = L"open_image_file_tool_strip_menu_item_";
      this->open_image_file_tool_strip_menu_item_->Size = System::Drawing::Size(160, 22);
      this->open_image_file_tool_strip_menu_item_->Text = L"Open Image File";
      // 
      // open_video_file_tool_strip_menu_item_
      // 
      this->open_video_file_tool_strip_menu_item_->Name = L"open_video_file_tool_strip_menu_item_";
      this->open_video_file_tool_strip_menu_item_->Size = System::Drawing::Size(160, 22);
      this->open_video_file_tool_strip_menu_item_->Text = L"Open Video File";
      // 
      // save_screen_tool_strip_menu_item_
      // 
      this->save_screen_tool_strip_menu_item_->Name = L"save_screen_tool_strip_menu_item_";
      this->save_screen_tool_strip_menu_item_->Size = System::Drawing::Size(160, 22);
      this->save_screen_tool_strip_menu_item_->Text = L"Save Screen";
      // 
      // record_screen_tool_strip_menu_item_
      // 
      this->record_screen_tool_strip_menu_item_->Name = L"record_screen_tool_strip_menu_item_";
      this->record_screen_tool_strip_menu_item_->Size = System::Drawing::Size(160, 22);
      this->record_screen_tool_strip_menu_item_->Text = L"Record Screen";
      // 
      // mode_tool_strip_menu_item_
      // 
      this->mode_tool_strip_menu_item_->Name = L"mode_tool_strip_menu_item_";
      this->mode_tool_strip_menu_item_->Size = System::Drawing::Size(50, 20);
      this->mode_tool_strip_menu_item_->Text = L"Mode";
      // 
      // view_tool_strip_menu_item_
      // 
      this->view_tool_strip_menu_item_->Name = L"view_tool_strip_menu_item_";
      this->view_tool_strip_menu_item_->Size = System::Drawing::Size(44, 20);
      this->view_tool_strip_menu_item_->Text = L"View";
      // 
      // grid_size_track_bar_
      // 
      this->grid_size_track_bar_->Location = System::Drawing::Point(10, 120);
      this->grid_size_track_bar_->Maximum = 200;
      this->grid_size_track_bar_->Minimum = 4;
      this->grid_size_track_bar_->Name = L"grid_size_track_bar_";
      this->grid_size_track_bar_->Size = System::Drawing::Size(100, 45);
      this->grid_size_track_bar_->TabIndex = 2;
      this->grid_size_track_bar_->TickStyle = System::Windows::Forms::TickStyle::None;
      this->grid_size_track_bar_->Value = 20;
      // 
      // grid_size_label_
      // 
      this->grid_size_label_->AutoSize = true;
      this->grid_size_label_->Location = System::Drawing::Point(15, 150);
      this->grid_size_label_->Name = L"grid_size_label_";
      this->grid_size_label_->Size = System::Drawing::Size(53, 13);
      this->grid_size_label_->TabIndex = 3;
      this->grid_size_label_->Text = L"Grid size :";
      // 
      // original_size_label_
      // 
      this->original_size_label_->AutoSize = true;
      this->original_size_label_->Location = System::Drawing::Point(15, 50);
      this->original_size_label_->Name = L"original_size_label_";
      this->original_size_label_->Size = System::Drawing::Size(69, 13);
      this->original_size_label_->TabIndex = 4;
      this->original_size_label_->Text = L"Original size :";
      // 
      // target_size_label_
      // 
      this->target_size_label_->AutoSize = true;
      this->target_size_label_->Location = System::Drawing::Point(15, 80);
      this->target_size_label_->Name = L"target_size_label_";
      this->target_size_label_->Size = System::Drawing::Size(65, 13);
      this->target_size_label_->TabIndex = 5;
      this->target_size_label_->Text = L"Target size :";
      // 
      // start_button_
      // 
      this->start_button_->Location = System::Drawing::Point(15, 300);
      this->start_button_->Name = L"start_button_";
      this->start_button_->Size = System::Drawing::Size(100, 25);
      this->start_button_->TabIndex = 6;
      this->start_button_->Text = L"Start";
      this->start_button_->UseVisualStyleBackColor = true;
      // 
      // show_lines_check_box_
      // 
      this->show_lines_check_box_->AutoSize = true;
      this->show_lines_check_box_->Location = System::Drawing::Point(15, 254);
      this->show_lines_check_box_->Name = L"show_lines_check_box_";
      this->show_lines_check_box_->Size = System::Drawing::Size(51, 17);
      this->show_lines_check_box_->TabIndex = 8;
      this->show_lines_check_box_->Text = L"Lines";
      this->show_lines_check_box_->UseVisualStyleBackColor = true;
      // 
      // show_image_check_box_
      // 
      this->show_image_check_box_->AutoSize = true;
      this->show_image_check_box_->Checked = true;
      this->show_image_check_box_->CheckState = System::Windows::Forms::CheckState::Checked;
      this->show_image_check_box_->Location = System::Drawing::Point(15, 231);
      this->show_image_check_box_->Name = L"show_image_check_box_";
      this->show_image_check_box_->Size = System::Drawing::Size(55, 17);
      this->show_image_check_box_->TabIndex = 9;
      this->show_image_check_box_->Text = L"Image";
      this->show_image_check_box_->UseVisualStyleBackColor = true;
      // 
      // target_width_numeric_up_down_
      // 
      this->target_width_numeric_up_down_->Location = System::Drawing::Point(15, 350);
      this->target_width_numeric_up_down_->Name = L"target_width_numeric_up_down_";
      this->target_width_numeric_up_down_->Size = System::Drawing::Size(120, 20);
      this->target_width_numeric_up_down_->TabIndex = 13;
      // 
      // target_height_numeric_up_down_
      // 
      this->target_height_numeric_up_down_->Location = System::Drawing::Point(15, 375);
      this->target_height_numeric_up_down_->Name = L"target_height_numeric_up_down_";
      this->target_height_numeric_up_down_->Size = System::Drawing::Size(120, 20);
      this->target_height_numeric_up_down_->TabIndex = 14;
      // 
      // focus_scale_label_
      // 
      this->focus_scale_label_->AutoSize = true;
      this->focus_scale_label_->Location = System::Drawing::Point(15, 210);
      this->focus_scale_label_->Name = L"focus_scale_label_";
      this->focus_scale_label_->Size = System::Drawing::Size(70, 13);
      this->focus_scale_label_->TabIndex = 16;
      this->focus_scale_label_->Text = L"Focus scale :";
      // 
      // focus_scale_track_bar_
      // 
      this->focus_scale_track_bar_->Location = System::Drawing::Point(10, 180);
      this->focus_scale_track_bar_->Maximum = 100;
      this->focus_scale_track_bar_->Minimum = -100;
      this->focus_scale_track_bar_->Name = L"focus_scale_track_bar_";
      this->focus_scale_track_bar_->Size = System::Drawing::Size(100, 45);
      this->focus_scale_track_bar_->TabIndex = 15;
      this->focus_scale_track_bar_->TickStyle = System::Windows::Forms::TickStyle::None;
      this->focus_scale_track_bar_->Value = 15;
      // 
      // focus_check_box_
      // 
      this->focus_check_box_->AutoSize = true;
      this->focus_check_box_->Checked = true;
      this->focus_check_box_->CheckState = System::Windows::Forms::CheckState::Checked;
      this->focus_check_box_->Location = System::Drawing::Point(15, 277);
      this->focus_check_box_->Name = L"focus_check_box_";
      this->focus_check_box_->Size = System::Drawing::Size(55, 17);
      this->focus_check_box_->TabIndex = 17;
      this->focus_check_box_->Text = L"Focus";
      this->focus_check_box_->UseVisualStyleBackColor = true;
      // 
      // GLForm
      // 
      this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
      this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
      this->ClientSize = System::Drawing::Size(1084, 662);
      this->Controls->Add(this->focus_check_box_);
      this->Controls->Add(this->focus_scale_label_);
      this->Controls->Add(this->focus_scale_track_bar_);
      this->Controls->Add(this->target_height_numeric_up_down_);
      this->Controls->Add(this->target_width_numeric_up_down_);
      this->Controls->Add(this->show_image_check_box_);
      this->Controls->Add(this->show_lines_check_box_);
      this->Controls->Add(this->start_button_);
      this->Controls->Add(this->target_size_label_);
      this->Controls->Add(this->original_size_label_);
      this->Controls->Add(this->grid_size_label_);
      this->Controls->Add(this->grid_size_track_bar_);
      this->Controls->Add(this->gl_panel_);
      this->Controls->Add(this->menu_strip_);
      this->DoubleBuffered = true;
      this->KeyPreview = true;
      this->MainMenuStrip = this->menu_strip_;
      this->Name = L"GLForm";
      this->Text = L"Content-Aware Retargeting";
      this->menu_strip_->ResumeLayout(false);
      this->menu_strip_->PerformLayout();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->grid_size_track_bar_))->EndInit();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->target_width_numeric_up_down_))->EndInit();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->target_height_numeric_up_down_))->EndInit();
      (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->focus_scale_track_bar_))->EndInit();
      this->ResumeLayout(false);
      this->PerformLayout();

    }
#pragma endregion
  };
}
